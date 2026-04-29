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
#ifndef FILESMODELITEM_H
#define FILESMODELITEM_H

#include <QString>
#include <cstdint>
#include <filesystem>
#include <string>
#include <tuple>

class FilesModelItem
{
public:
  FilesModelItem();

  FilesModelItem(const FilesModelItem &other);

  FilesModelItem(FilesModelItem &&other);

  FilesModelItem(
      const std::tuple<std::string, uint64_t, uint64_t> &archive_item);

  FilesModelItem(const std::filesystem::path &source_path);

  FilesModelItem &
  operator=(const FilesModelItem &other);

  FilesModelItem &
  operator=(FilesModelItem &&other);

  QString source_path;
  QString path_in_archive;  

  bool exists_in_archive = false;

  uint64_t source_size = 0;
};

#endif // FILESMODELITEM_H
