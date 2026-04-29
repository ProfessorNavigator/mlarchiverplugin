/*
 * Copyright (C) 2026 Yury Bobylev <bobilev_yury@mail.ru>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <MainWindow.h>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QStyleOption>
#include <QVBoxLayout>
#include <StyledWindow.h>
#include <thread>

MainWindow::MainWindow(QWidget *parent,
                       const std::shared_ptr<MLBookProc> &mlbp)
    : QWidget(parent)
{
  this->mlbp = mlbp;

  translator = new QTranslator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for(const QString &locale : uiLanguages)
    {
      const QString baseName = "MLArchiverPlugin_" + QLocale(locale).name();
      if(translator->load(":/i18n/" + baseName))
        {
          qApp->installTranslator(translator);
          break;
        }
    }

  this->setWindowFlag(Qt::Window, true);
  this->setWindowTitle(tr("Archiver"));
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setWindowModality(Qt::WindowModal);

  this->setObjectName("Window");
}

MainWindow::~MainWindow()
{
  delete translator;
}

void
MainWindow::createWindow()
{
  QVBoxLayout *v_box = new QVBoxLayout;
  this->setLayout(v_box);

  QLabel *lab = new QLabel;
  lab->setObjectName("Label");
  lab->setText(tr("Archive path:"));
  v_box->addWidget(lab, 0, Qt::AlignLeft);

  archive_path = new QLineEdit;
  archive_path->setObjectName("LineEdit");
  v_box->addWidget(archive_path);

  QPushButton *open = new QPushButton;
  open->setText(tr("Open"));
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(open);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  open->setGraphicsEffect(shadow);
  open->setObjectName("ApplyButton");
  connect(open, &QPushButton::clicked, this, &MainWindow::openArchiveDialog);
  v_box->addWidget(open, 0, Qt::AlignRight | Qt::AlignVCenter);

  add_to_existing = new QCheckBox;
  add_to_existing->setObjectName("CheckBox");
  add_to_existing->setText(tr("Edit archive if it exists"));
  v_box->addWidget(add_to_existing, 0, Qt::AlignLeft);

  QHBoxLayout *h_box = new QHBoxLayout;
  v_box->addLayout(h_box);

  QPushButton *apply = new QPushButton;
  apply->setText(tr("Apply"));
  shadow = new QGraphicsDropShadowEffect(apply);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  apply->setGraphicsEffect(shadow);
  apply->setObjectName("ApplyButton");
  connect(apply, &QPushButton::clicked, this, &MainWindow::checkParameters);
  h_box->addWidget(apply, 0, Qt::AlignCenter);

  QPushButton *cancel = new QPushButton;
  cancel->setText(tr("Cancel"));
  shadow = new QGraphicsDropShadowEffect(cancel);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  cancel->setGraphicsEffect(shadow);
  cancel->setObjectName("CancelButton");
  connect(cancel, &QPushButton::clicked, this, &MainWindow::close);
  h_box->addWidget(cancel, 0, Qt::AlignCenter);

  QScreen *screen = this->parentWidget()->screen();
  QSize av_sz = screen->availableSize();
  av_sz.setHeight(this->sizeHint().height());
  av_sz.setWidth(av_sz.width() * 0.5);
  this->resize(av_sz);
}

void
MainWindow::openArchiveDialog()
{
  QFileDialog *fd = new QFileDialog(this);
  fd->setAttribute(Qt::WA_DeleteOnClose);
  fd->setWindowModality(Qt::WindowModal);

  fd->setDirectory(QDir::homePath());
  fd->setAcceptMode(QFileDialog::AcceptSave);

  std::vector<std::string> sup = mlbp->getSupportedArchivesTypesPacking();
  if(sup.size() > 0)
    {
      fd->setDefaultSuffix(sup.begin()->c_str());
    }

  QStringList filters;
  for(auto it = sup.begin(); it != sup.end(); it++)
    {
      QString filter = "*.";
      filter += it->c_str();
      filters.append(filter);
    }
  fd->setNameFilters(filters);

  connect(fd, &QFileDialog::fileSelected, this,
          [this](const QString &file)
            {
              archive_path->setText(file);
            });

  fd->show();
}

void
MainWindow::checkParameters()
{
  std::string str = archive_path->text().toStdString();
  if(str.empty())
    {
      errorDialog(ErrorType::PathEmpty);
      return void();
    }

  std::filesystem::path arch_path = std::u8string(str.begin(), str.end());
  std::string ext = mlbp->getExtension(arch_path);
  std::string find_str(".");
  std::string::size_type n = ext.find(find_str);
  if(n != std::u8string::npos)
    {
      ext.erase(0, n + find_str.size());
    }

  std::vector<std::string> sup = mlbp->getSupportedArchivesTypesPacking();
  auto it = std::find(sup.begin(), sup.end(), ext);
  if(it == sup.end())
    {
      errorDialog(ErrorType::IncorrectExtension, sup);
      return void();
    }

  bool add_existing = false;
  if(add_to_existing->checkState() == Qt::Checked)
    {
      add_existing = true;
    }
  AddFilesWindow *afw = new AddFilesWindow(this, add_existing, mlbp);
  procWindow(afw);

  connect(this, &MainWindow::signalCreatAddFilesWindow, afw,
          [afw, arch_path]
            {
              afw->createWindow(arch_path);
              afw->show();
            });
  std::thread thr(
      [this]
        {
          emit signalCreatAddFilesWindow();
        });
  thr.detach();
}

void
MainWindow::errorDialog(const ErrorType &er,
                        const std::vector<std::string> &supported_extensions)
{
  StyledWindow *window = new StyledWindow(this);
  window->setWindowModality(Qt::WindowModal);
  window->setObjectName("Window");

  QVBoxLayout *v_box = new QVBoxLayout;
  window->setLayout(v_box);

  switch(er)
    {
    case ErrorType::PathEmpty:
      {
        QLabel *lab = new QLabel;
        lab->setObjectName("Label");
        lab->setText(tr("Archive path is empty!"));
        lab->setAlignment(Qt::AlignCenter);
        v_box->addWidget(lab);
        break;
      }
    case ErrorType::IncorrectExtension:
      {
        QLabel *lab = new QLabel;
        lab->setObjectName("Label");
        lab->setText(tr("Incorrect archive extesinon! Extension must be one "
                        "of the following:"));
        lab->setAlignment(Qt::AlignCenter);
        v_box->addWidget(lab);

        QString str;
        for(auto it = supported_extensions.begin();
            it != supported_extensions.end(); it++)
          {
            if(!str.isEmpty())
              {
                str += ", ";
              }
            str += ".";
            str += it->c_str();
          }

        lab = new QLabel;
        lab->setObjectName("Label");
        lab->setWordWrap(true);
        lab->setText(str);
        lab->setAlignment(Qt::AlignCenter);
        v_box->addWidget(lab);
        break;
      }
    default:
      break;
    }

  QPushButton *close = new QPushButton;
  close->setText(tr("Close"));
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(close);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  close->setGraphicsEffect(shadow);
  close->setObjectName("ApplyButton");
  connect(close, &QPushButton::clicked, window, &StyledWindow::close);
  v_box->addWidget(close, 0, Qt::AlignCenter);

  window->show();
}

void
MainWindow::paintEvent(QPaintEvent *event)
{
  QStyleOption opt;
  opt.initFrom(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
MainWindow::procWindow(AddFilesWindow *afw)
{
  StyledWindow *window = new StyledWindow(this);
  window->setWindowModality(Qt::WindowModal);
  window->setObjectName("Window");

  window->allowClose(false);

  QVBoxLayout *v_box = new QVBoxLayout;
  window->setLayout(v_box);

  QLabel *lab = new QLabel;
  lab->setObjectName("Label");
  lab->setText(tr("Loading archive..."));
  lab->setAlignment(Qt::AlignCenter);
  v_box->addWidget(lab);

  connect(afw, &AddFilesWindow::signalWindowCreated, window,
          [window]
            {
              window->allowClose(true);
              window->close();
            });

  window->show();
}