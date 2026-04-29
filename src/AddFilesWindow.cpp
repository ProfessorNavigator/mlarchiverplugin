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

#include <AddFilesWindow.h>
#include <LibArchive.h>
#include <QFileDialog>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QStyleOption>
#include <QVBoxLayout>
#include <StyledWindow.h>
#include <iostream>
#include <thread>

AddFilesWindow::AddFilesWindow(QWidget *parent, const bool &add_to_existing,
                               const std::shared_ptr<MLBookProc> &mlbp)
    : QWidget(parent)
{
  this->add_to_existing = add_to_existing;
  this->mlbp = mlbp;

  this->setWindowFlag(Qt::Window);
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setWindowTitle(tr("Files"));
  this->setWindowModality(Qt::WindowModal);

  this->setObjectName("Window");
}

AddFilesWindow::~AddFilesWindow()
{
  delete model;
}

void
AddFilesWindow::createWindow(const std::filesystem::path &archive_path)
{
  this->archive_path = archive_path;

  std::vector<std::tuple<std::string, uint64_t, uint64_t>> files;
  if(add_to_existing)
    {
      std::unique_ptr<LibArchive> la(new LibArchive(mlbp));
      try
        {
          la->listFilesInZip(archive_path, files);
        }
      catch(std::exception &er)
        {
          std::cout << "AddFilesWindow::createWindow: \"" << er.what() << "\""
                    << std::endl;
        }
    }

  QVBoxLayout *v_box = new QVBoxLayout;
  this->setLayout(v_box);

  model = new FilesModel(nullptr, files);

  table = new TableView;
  table->setObjectName("Table");
  table->viewport()->setObjectName("TableViewport");
  connect(table, &TableView::signalResized, this,
          [this](const QSize &sz)
            {
              QHeaderView *hh = table->horizontalHeader();
              hh->resizeSection(0, sz.width() * 0.5);
              hh->resizeSection(1, sz.width() - hh->sectionSize(0));
            });
  connect(table, &TableView::signalLeftMouseButton, this,
          [this](const QPoint &global)
            {
              QModelIndex index
                  = table->indexAt(table->viewport()->mapFromGlobal(global));
              table->setCurrentIndex(index);
            });
  QAbstractItemModel *prev = table->model();
  table->setModel(model);
  delete prev;
  table->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(table, &TableView::customContextMenuRequested, this,
          [this](const QPoint &pos)
            {
              QModelIndex index = table->indexAt(pos);
              table->setCurrentIndex(index);
              QMenu *menu = new QMenu(table);
              menu->setObjectName("Menu");
              menu->setAttribute(Qt::WA_DeleteOnClose);
              menu->addActions(table->actions());
              menu->popup(table->viewport()->mapToGlobal(pos));
            });
  v_box->addWidget(table);

  QList<QAction *> actions;

  QAction *add_files_act = new QAction(tr("Add files"));
  connect(add_files_act, &QAction::triggered, this,
          &AddFilesWindow::addFilesDialog);
  actions.append(add_files_act);

  QAction *add_directory_act = new QAction(tr("Add directory"));
  connect(add_directory_act, &QAction::triggered, this,
          &AddFilesWindow::addDirectoryDialog);
  actions.append(add_directory_act);

  QAction *remove_entry_act = new QAction(tr("Remove entry"));
  connect(remove_entry_act, &QAction::triggered, this,
          [this]
            {
              QModelIndex index = table->currentIndex();
              model->removeEntry(index);
            });
  actions.append(remove_entry_act);

  table->addActions(actions);

  connect(table, &TableView::destroyed,
          [actions]
            {
              for(qsizetype i = 0; i < actions.size(); i++)
                {
                  delete actions[i];
                }
            });

  QHBoxLayout *h_box = new QHBoxLayout;
  v_box->addLayout(h_box);

  QPushButton *add_files = new QPushButton;
  add_files->setText(tr("Add files"));
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(add_files);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  add_files->setGraphicsEffect(shadow);
  add_files->setObjectName("ApplyButton");
  connect(add_files, &QPushButton::clicked, add_files_act, &QAction::trigger);
  h_box->addWidget(add_files, 0, Qt::AlignCenter);

  QPushButton *add_directory = new QPushButton;
  add_directory->setText(tr("Add directory"));
  shadow = new QGraphicsDropShadowEffect(add_directory);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  add_directory->setGraphicsEffect(shadow);
  add_directory->setObjectName("ApplyButton");
  connect(add_directory, &QPushButton::clicked, add_directory_act,
          &QAction::trigger);
  h_box->addWidget(add_directory, 0, Qt::AlignCenter);

  QPushButton *remove_selected = new QPushButton;
  remove_selected->setText(tr("Remove selected"));
  shadow = new QGraphicsDropShadowEffect(remove_selected);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  remove_selected->setGraphicsEffect(shadow);
  remove_selected->setObjectName("ClearButton");
  connect(remove_selected, &QPushButton::clicked, remove_entry_act,
          &QAction::trigger);
  h_box->addWidget(remove_selected, 0, Qt::AlignCenter);

  QPushButton *reset_to_begin = new QPushButton;
  reset_to_begin->setText(tr("Reset to begin"));
  shadow = new QGraphicsDropShadowEffect(reset_to_begin);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  reset_to_begin->setGraphicsEffect(shadow);
  reset_to_begin->setObjectName("ClearButton");
  connect(reset_to_begin, &QPushButton::clicked, this,
          [this]
            {
              model->resetToDefault();
            });
  h_box->addWidget(reset_to_begin, 0, Qt::AlignCenter);

  QFrame *frame = new QFrame;
  frame->setFrameShape(QFrame::HLine);
  v_box->addWidget(frame);

  h_box = new QHBoxLayout;
  v_box->addLayout(h_box);

  QPushButton *write = new QPushButton;
  write->setText(tr("Write archive"));
  shadow = new QGraphicsDropShadowEffect(write);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  write->setGraphicsEffect(shadow);
  write->setObjectName("ApplyButton");
  connect(write, &QPushButton::clicked, this, &AddFilesWindow::checkInput);
  h_box->addWidget(write, 0, Qt::AlignCenter);

  QPushButton *cancel = new QPushButton;
  cancel->setText(tr("Cancel"));
  shadow = new QGraphicsDropShadowEffect(cancel);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  cancel->setGraphicsEffect(shadow);
  cancel->setObjectName("CancelButton");
  connect(cancel, &QPushButton::clicked, this, &AddFilesWindow::close);
  h_box->addWidget(cancel, 0, Qt::AlignCenter);

  QScreen *screen = this->parentWidget()->screen();
  QSize av_sz = screen->availableSize();
  av_sz.setHeight(av_sz.height() * 0.5);
  av_sz.setWidth(av_sz.width() * 0.5);
  this->resize(av_sz);

  emit signalWindowCreated();
}

void
AddFilesWindow::paintEvent(QPaintEvent *event)
{
  QStyleOption opt;
  opt.initFrom(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
AddFilesWindow::addFilesDialog()
{
  QFileDialog *fd = new QFileDialog(this);
  fd->setAttribute(Qt::WA_DeleteOnClose);
  fd->setWindowModality(Qt::WindowModal);

  fd->setDirectory(QDir::homePath());
  fd->setAcceptMode(QFileDialog::AcceptOpen);
  fd->setFileMode(QFileDialog::ExistingFiles);

  connect(fd, &QFileDialog::filesSelected, model, &FilesModel::addFiles);

  fd->show();
}

void
AddFilesWindow::addDirectoryDialog()
{
  QFileDialog *fd = new QFileDialog(this);
  fd->setAttribute(Qt::WA_DeleteOnClose);
  fd->setWindowModality(Qt::WindowModal);

  fd->setDirectory(QDir::homePath());
  fd->setAcceptMode(QFileDialog::AcceptOpen);
  fd->setFileMode(QFileDialog::Directory);

  connect(fd, &QFileDialog::fileSelected, model, &FilesModel::addDirectory);

  fd->show();
}

void
AddFilesWindow::checkInput()
{
  std::vector<FilesModelItem> items = model->getModel();
  auto it = std::find_if(items.begin(), items.end(),
                         [](const FilesModelItem &el)
                           {
                             return el.exists_in_archive;
                           });
  if(it != items.end())
    {
      errorDialog(ErrorType::FnmConflict, "");
      return void();
    }
  wa = std::make_shared<WriteArchive>(mlbp, archive_path, items,
                                      add_to_existing);
  confirmationDialog();
}

void
AddFilesWindow::createProcessWindow()
{
  StyledWindow *window = new StyledWindow(this);
  window->setWindowModality(Qt::WindowModal);
  window->setObjectName("Window");
  window->allowClose(false);

  QVBoxLayout *v_box = new QVBoxLayout;
  window->setLayout(v_box);

  QLabel *lab = new QLabel;
  lab->setObjectName("Label");
  lab->setText(tr("Writing..."));
  lab->setAlignment(Qt::AlignCenter);
  v_box->addWidget(lab);

  QProgressBar *progress = new QProgressBar;
  progress->setObjectName("ProgressBar");
  progress->setMinimum(0);
  progress->setMaximum(1000);
  v_box->addWidget(progress);

  connect(this, &AddFilesWindow::signalProgress, progress,
          [progress](const double &progr, const double &total)
            {
              int val = static_cast<int>(progr * 1000.0 / total);
              progress->setValue(val);
            });

  wa->signal_progress = [this](double progr, double total)
    {
      emit signalProgress(progr, total);
    };

  QPushButton *cancel = new QPushButton;
  cancel->setText(tr("Cancel"));
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(cancel);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  cancel->setGraphicsEffect(shadow);
  cancel->setObjectName("CancelButton");
  connect(cancel, &QPushButton::clicked, this,
          [this, lab, progress, cancel]
            {
              lab->setText(tr("Cancelling..."));
              progress->setVisible(false);
              cancel->setVisible(false);
              wa->cancelAll();
            });
  v_box->addWidget(cancel, 0, Qt::AlignCenter);

  window->show();

  connect(this, &AddFilesWindow::signalError, window,
          [this, window](const std::string &er)
            {
              window->allowClose(true);
              window->close();
              errorDialog(ErrorType::Error, er.c_str());
            });

  connect(this, &AddFilesWindow::signalFinished, window,
          [this, window]
            {
              window->allowClose(true);
              window->close();
              errorDialog(ErrorType::Success, "");
            });

  std::thread thr(
      [this]
        {
          try
            {
              wa->writeArchive();
            }
          catch(std::exception &er)
            {
              std::string str(er.what());
              emit signalError(str);
              return void();
            }
          emit signalFinished();
        });
  thr.detach();
}

void
AddFilesWindow::errorDialog(const ErrorType &er, const QString &txt)
{
  StyledWindow *window;
  if(er == ErrorType::Success)
    {
      window = new StyledWindow(this->parentWidget());
    }
  else
    {
      window = new StyledWindow(this);
    }
  window->setWindowModality(Qt::WindowModal);
  window->setObjectName("Window");

  QVBoxLayout *v_box = new QVBoxLayout;
  window->setLayout(v_box);

  switch(er)
    {
    case ErrorType::FnmConflict:
      {
        QLabel *lab = new QLabel;
        lab->setObjectName("Label");
        lab->setText(tr("File names in archive has conflicts!"));
        lab->setAlignment(Qt::AlignCenter);
        v_box->addWidget(lab);
        break;
      }
    case ErrorType::Error:
      {
        QLabel *lab = new QLabel;
        lab->setObjectName("Label");
        lab->setText(tr("Error!"));
        lab->setAlignment(Qt::AlignCenter);
        v_box->addWidget(lab);

        lab = new QLabel;
        lab->setObjectName("Label");
        lab->setWordWrap(true);
        lab->setText(txt);
        lab->setAlignment(Qt::AlignCenter);
        v_box->addWidget(lab);
        break;
      }
    case ErrorType::Success:
      {
        QLabel *lab = new QLabel;
        lab->setObjectName("Label");
        lab->setText(tr("Archive has been created!"));
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

  if(er == ErrorType::Success)
    {
      this->close();
    }
}

void
AddFilesWindow::confirmationDialog()
{
  StyledWindow *window = new StyledWindow(this);
  window->setWindowModality(Qt::WindowModal);
  window->setObjectName("Window");

  QVBoxLayout *v_box = new QVBoxLayout;
  window->setLayout(v_box);

  QLabel *lab = new QLabel;
  lab->setObjectName("Label");
  lab->setText(tr("Are you sure?"));
  lab->setAlignment(Qt::AlignCenter);
  v_box->addWidget(lab);

  QHBoxLayout *h_box = new QHBoxLayout;
  v_box->addLayout(h_box);

  QPushButton *yes = new QPushButton;
  yes->setText(tr("Yes"));
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(yes);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  yes->setGraphicsEffect(shadow);
  yes->setObjectName("ApplyButton");
  connect(yes, &QPushButton::clicked, this,
          [this, window]
            {
              createProcessWindow();
              window->close();
            });
  h_box->addWidget(yes, 0, Qt::AlignCenter);

  QPushButton *no = new QPushButton;
  no->setText(tr("No"));
  shadow = new QGraphicsDropShadowEffect(no);
  shadow->setBlurRadius(12);
  shadow->setOffset(0, 2);
  shadow->setColor(QColor(0, 0, 0, 120));
  no->setGraphicsEffect(shadow);
  no->setObjectName("CancelButton");
  connect(no, &QPushButton::clicked, window, &StyledWindow::close);
  h_box->addWidget(no, 0, Qt::AlignCenter);

  window->show();
}
