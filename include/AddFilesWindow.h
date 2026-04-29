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
#ifndef ADDFILESWINDOW_H
#define ADDFILESWINDOW_H

#include <FilesModel.h>
#include <MLBookProc.h>
#include <QPaintEvent>
#include <QProgressBar>
#include <QWidget>
#include <TableView.h>
#include <WriteArchive.h>
#include <filesystem>

class AddFilesWindow : public QWidget
{
  Q_OBJECT
public:
  AddFilesWindow(QWidget *parent, const bool &add_to_existing,
                 const std::shared_ptr<MLBookProc> &mlbp);

  virtual ~AddFilesWindow();

  void
  createWindow(const std::filesystem::path &archive_path);

signals:
  void
  signalWindowCreated();

private:
  void
  paintEvent(QPaintEvent *event) override;

  void
  addFilesDialog();

  void
  addDirectoryDialog();

  void
  checkInput();

  void
  createProcessWindow();

  enum ErrorType
  {
    FnmConflict,
    Error,
    Success
  };

  void
  errorDialog(const ErrorType &er, const QString &txt);

  void
  confirmationDialog();

  bool add_to_existing = false;
  std::shared_ptr<MLBookProc> mlbp;

  std::filesystem::path archive_path;

  FilesModel *model = nullptr;

  TableView *table;

  std::shared_ptr<WriteArchive> wa;

signals:
  void
  signalProgress(const double &progress, const double &total);

  void
  signalError(const std::string &er);

  void
  signalFinished();
};

#endif // ADDFILESWINDOW_H
