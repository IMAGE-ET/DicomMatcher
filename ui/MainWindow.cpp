//
// Created by agabor on 2017.09.11..
//

#include "MainWindow.h"

#include <QtCore/QStringListModel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QProgressDialog>
#include <QtCore/QCoreApplication>

#include "../cv/DicomReader.h"

#include "ProgressDialog.h"

using namespace cv;
using namespace std;
using namespace Qt;

void MainWindow::setImage(Mat &image) {
    imageLabel->setFixedSize(image.cols, image.rows);
    auto format = image.type() == CV_8U ? QImage::Format_Grayscale8 : QImage::Format_RGB888;
    const QImage &qImage = QImage(image.data, image.cols, image.rows, int(image.step), format);
    imageLabel->setPixmap(QPixmap::fromImage(qImage));
    adjustSize();
}

MainWindow::MainWindow(std::vector<std::shared_ptr<Image>> images) {
    this->images = images;

    imageLabel = new QLabel;
    setCentralWidget(imageLabel);

    initFilesWidget();

    initMatchesWidget();

    initFilterButton();

    initConfigWidget();

    setFiles(images);
}

void MainWindow::initConfigWidget() {
    configWidget = new ConfigWidget(this);
    configWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(TopDockWidgetArea, configWidget);
}

void MainWindow::setFiles(const vector<shared_ptr<Image>> &images) {
    QStringList fileNames;
    for (const auto &img : images) {
        fileNames << img->file_name.c_str();
    }
    auto m = new QStringListModel();
    m->setStringList(fileNames);
    filesView->setModel(m);
    currentImage = images[0];
    setImage(currentImage->original);
}

void MainWindow::initFilterButton() {
    QDockWidget *button_dock = new QDockWidget(this);
    button_dock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    QPushButton *button = new QPushButton(QObject::tr("Filter"), button_dock);
    button_dock->setWidget(button);

    addDockWidget(BottomDockWidgetArea, button_dock);

    connect(button, &QPushButton::pressed, this, &MainWindow::runSURF);
}

void MainWindow::initMatchesWidget() {
    filteredView = createDockedListView(QObject::tr("Matches"), RightDockWidgetArea);

    connect(filteredView, &QListView::activated,[=]( const QModelIndex &index ) {
        setImage(matches[index.row()].image_b->original);
    });

    connect(filteredView, &QListView::doubleClicked,[=]( const QModelIndex &index ) {
        cv::imshow("Matched Features", matches[index.row()].matchImage());
    });
}

QListView *MainWindow::createDockedListView(const QString &name, DockWidgetArea area) {
    auto *dockWidget = new QDockWidget(name, this);
    dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);

    auto *listView = new QListView(dockWidget);
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listView->setFixedWidth(200);

    dockWidget->setWidget(listView);
    addDockWidget(area, dockWidget);

    return listView;
}

void MainWindow::initFilesWidget() {
    filesView = createDockedListView(QObject::tr("Files"), LeftDockWidgetArea);

    connect(filesView, &QListView::activated,[=]( const QModelIndex &index ) {
        currentImage = images[index.row()];
        setImage(currentImage->original);
    });
}

void MainWindow::runSURF() {
    if (isRunning)
        return;
    isRunning = true;

    MatchSettings settings = configWidget->settings;
    matches.clear();
    if (configWidget->changed) {
        runFeatureDetectionAndDescription(settings);
    }
    runFeatureMatching(settings);

    isRunning = false;
}

void MainWindow::runFeatureMatching(MatchSettings &settings) {
    ProgressDialog::start("Feature Matching", static_cast<int>(images.size() - 1));

    for (const auto &img : images) {
        if (img == currentImage)
            continue;

        ImagePair imagePair = getImagePair(img, settings);

        if (imagePair.matchCount() > 0) {
            matches.push_back(imagePair);
        }
        ProgressDialog::step();
    }

    ProgressDialog::end();

    sortMatches();

    setMatchLabels();
}

void MainWindow::sortMatches() {
    sort(matches.begin(), matches.end(),
         [](const ImagePair & a, const ImagePair & b) -> bool
         {
             return a.matchCount() > b.matchCount();
         });
}

void MainWindow::setMatchLabels() const {
    QStringList labels;
    for (auto &pair : matches) {
        labels << pair.label().c_str();
    }
    auto model = new QStringListModel();
    model->setStringList(labels);
    filteredView->setModel(model);
}

ImagePair MainWindow::getImagePair(const shared_ptr<Image> &img, MatchSettings &settings) const {
    ImagePair imagePair(settings, currentImage, img);

    if (settings.mirrorY) {
        ImagePair mirroredPair(settings, currentImage, img->mirrored);
        if (mirroredPair.matchCount() > imagePair.matchCount())
            return mirroredPair;
    }
    return imagePair;
}

void MainWindow::runFeatureDetectionAndDescription(MatchSettings &settings) {
    configWidget->changed = false;

    ProgressDialog::start("Feature Detection & Description", static_cast<int>(images.size()));

    for (const auto &img : images) {
        img->scan(settings);
        ProgressDialog::step();
    }

    ProgressDialog::end();
}

