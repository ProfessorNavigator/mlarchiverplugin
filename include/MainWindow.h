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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <AddFilesWindow.h>
#include <MLBookProc.h>
#include <QCheckBox>
#include <QLineEdit>
#include <QPaintEvent>
#include <QTranslator>
#include <QWidget>

class MainWindow : public QWidget
{
  Q_OBJECT
public:
  MainWindow(QWidget *parent, const std::shared_ptr<MLBookProc> &mlbp);

  virtual ~MainWindow();

  void
  createWindow();

private:
  void
  openArchiveDialog();

  void
  checkParameters();

  enum ErrorType
  {
    PathEmpty,
    IncorrectExtension
  };

  void
  errorDialog(const ErrorType &er,
              const std::vector<std::string> &supported_extensions
              = std::vector<std::string>());

  void
  paintEvent(QPaintEvent *event) override;

  void
  procWindow(AddFilesWindow *afw);

  std::shared_ptr<MLBookProc> mlbp;
  QTranslator *translator;

  QLineEdit *archive_path;
  QCheckBox *add_to_existing;

signals:
  void
  signalCreatAddFilesWindow();
};

#endif // MAINWINDOW_H
