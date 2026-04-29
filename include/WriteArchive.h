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
#ifndef WRITEARCHIVE_H
#define WRITEARCHIVE_H

#include <FilesModelItem.h>
#include <LibArchive.h>
#include <atomic>
#include <filesystem>
#include <functional>
#include <vector>

class WriteArchive : public LibArchive
{
public:
  WriteArchive(const std::shared_ptr<MLBookProc> &mlbp,
               const std::filesystem::path &archive_path,
               const std::vector<FilesModelItem> &files,
               const bool &overwrite);

  void
  writeArchive();

  void
  cancelAll();

  bool
  getCanceled();

  std::function<void(double progress, double total)> signal_progress;

private:
  void
  processOriginalArchive();

  std::filesystem::path archive_path;
  std::vector<FilesModelItem> files;
  bool overwrite = false;

  std::filesystem::path tmp;
  std::shared_ptr<archive> a_write;

  double total = 0.0;
  double progress = 0.0;

  std::atomic<bool> cancel;
};

#endif // WRITEARCHIVE_H
