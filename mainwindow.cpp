#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDirIterator>
#include <QCryptographicHash>
#include <ctime>

main_window::main_window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    QCommonStyle style;
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));

    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);

    //scan_directory(QDir::homePath());
}

main_window::~main_window() {

}

void main_window::select_directory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    scan_directory(dir);
}

void main_window::scan_directory(QString const& dir) {
    ui->statusBar->showMessage("Starting duplicates search ...");
    clock_t begin = std::clock();
    ui->treeWidget->clear();

    if (!find_duplicates(dir)) {
        show_no_dups();
    }

    setWindowTitle(QString("Duplicates in directory - %1").arg(dir));
    clock_t end = std::clock();
    double seconds = double(end - begin) / CLOCKS_PER_SEC;
    ui->statusBar->showMessage(QString("Finished in ") + QString::number(seconds) + QString("sec"));
}

void main_window::show_no_dups() {
     QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
     item->setText(0, "Found no duplicates");
     ui->treeWidget->addTopLevelItem(item);
}

bool main_window::find_duplicates(QString const& dir) {
    // getting all filenames
    QDirIterator it(dir, QDir::Hidden | QDir::Files, QDirIterator::Subdirectories);
    QVector<QString> files;
    while (it.hasNext()) {
        files.push_back(it.next());
    }

    // grouping
    QMap<qint64, QVector<QString>> map;
    for (QString filename : files) {
        qint64 filesize = (new QFileInfo(filename))->size();
        auto it = map.find(filesize);
        if (it != map.end()) {
            it->push_back(filename);
        } else {
            QVector<QString> temp;
            temp.push_back(filename);
            map.insert(filesize, temp);
        }
    }

    // finding duplicates
    bool flag = false;
    int curState = 1;
    QCryptographicHash sha(QCryptographicHash::Sha3_256);
    for (auto it = map.begin(); it != map.end(); ++it, ++curState) {
        if (it->size() <= 1) {
            continue;
        }
        QMap<QByteArray, QVector<QFileInfo*>> hashes;
        for (QString filename : *it) {
           sha.reset();
           QFile file(filename);
           if (file.open(QIODevice::ReadOnly)) {
               sha.addData(&file);
           }
           QByteArray res = sha.result();
           auto st = hashes.find(res);
           if (st != hashes.end()) {
               st->push_back(new QFileInfo(filename));
           } else {
               QVector<QFileInfo*> temp;
               temp.push_back(new QFileInfo(filename));
               hashes.insert(res, temp);
           }
        }
        for (auto st = hashes.begin(); st != hashes.end(); ++st) {
            if (st->size() > 1) {
                // adding to the tree
                flag = true;
                QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
                item->setText(0, QString("Found ") + QString::number(st->size()) + QString(" duplicates"));
                item->setText(1, QString::number(st->front()->size()) + QString(" bytes"));
                for (QFileInfo* dup : *st) {
                   QTreeWidgetItem* childItem = new QTreeWidgetItem();
                   childItem->setText(0, dup->filePath());
                   item->addChild(childItem);
                }
                ui->treeWidget->addTopLevelItem(item);
            }
        }
        ui->progressBar->setValue(100 * curState / map.size());
    }

    // finding duplicates
    return flag;
}

void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}
