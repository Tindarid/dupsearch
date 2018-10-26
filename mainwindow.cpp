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
    ui->treeWidget->clear();
    setWindowTitle(QString("Duplicates in directory - %1").arg(dir));

    // getting all filenames
    QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
    QVector<QString> files;
    while (it.hasNext()) {
        files.push_back(it.next());
    }

    // generating hashes
    QVector<QByteArray> hashes;
    QCryptographicHash sha(QCryptographicHash::Sha256);
    for (QString filename : files) {
        sha.reset();
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            sha.addData(file.readAll());
        }
        hashes.push_back(sha.result());
    }

    // finding duplicates
    QVector<bool> used(files.size(), false);
    bool flag = false;
    for (int i = 0; i < files.size(); ++i) {
        if (used[i]) {
            continue;
        }
        QVector<QFileInfo*> duplicates;
        for (int j = i + 1; j < files.size(); ++j) {
            if (!used[j] && hashes[i] == hashes[j]) {
                duplicates.push_back(new QFileInfo(files[j]));
                used[j] = true;
            }
        }
        if (!duplicates.empty()) {
            // adding to the tree
            flag = true;
            QFileInfo* parent = new QFileInfo(files[i]);
            duplicates.push_back(parent);
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
            item->setText(0, QString("Found ") + QString::number(duplicates.size()) + QString(" duplicates"));
            item->setText(1, QString::number(parent->size()) + QString(" bytes"));
            for (QFileInfo* dup : duplicates) {
               QTreeWidgetItem* childItem = new QTreeWidgetItem();
               childItem->setText(0, dup->filePath());
               item->addChild(childItem);
            }
            ui->treeWidget->addTopLevelItem(item);
        }
    }

    // if no duplicates were found
    if (!flag) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, "Found no duplicates");
        ui->treeWidget->addTopLevelItem(item);

    }
}

void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}
