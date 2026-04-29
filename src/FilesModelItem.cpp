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

#include <FilesModelItem.h>
#include <iostream>

FilesModelItem::FilesModelItem()
{
}

FilesModelItem::FilesModelItem(const FilesModelItem &other)
{
  source_path = other.source_path;
  path_in_archive = other.path_in_archive;
  exists_in_archive = other.exists_in_archive;
  source_size = other.source_size;
}

FilesModelItem::FilesModelItem(FilesModelItem &&other)
{
  source_path = std::move(other.source_path);
  path_in_archive = std::move(other.path_in_archive);
  exists_in_archive = std::move(other.exists_in_archive);
  source_size = std::move(other.source_size);
}

FilesModelItem::FilesModelItem(
    const std::tuple<std::string, uint64_t, uint64_t> &archive_item)
{
  path_in_archive = std::get<0>(archive_item).c_str();
  source_size = std::get<1>(archive_item);
}

FilesModelItem::FilesModelItem(const std::filesystem::path &source_path)
{
  std::u8string u8str = source_path.u8string();
  this->source_path = u8str.c_str();
  u8str = source_path.filename().u8string();
#ifdef _WIN32
  std::replace(u8str.begin(), u8str.end(), '\\', '/');
#endif
  this->path_in_archive = u8str.c_str();
  std::error_code ec;
  source_size
      = static_cast<uint64_t>(std::filesystem::file_size(source_path, ec));
  if(ec)
    {
      std::cout << "FilesModelItem::FilesModelItem: \"" << ec.message()
                << "\" " << source_path << std::endl;
    }
}

FilesModelItem &
FilesModelItem::operator=(const FilesModelItem &other)
{
  if(this != &other)
    {
      source_path = other.source_path;
      path_in_archive = other.path_in_archive;
      exists_in_archive = other.exists_in_archive;
      source_size = other.source_size;
    }

  return *this;
}

FilesModelItem &
FilesModelItem::operator=(FilesModelItem &&other)
{
  if(this != &other)
    {
      source_path = std::move(other.source_path);
      path_in_archive = std::move(other.path_in_archive);
      exists_in_archive = std::move(other.exists_in_archive);
      source_size = std::move(other.source_size);
    }

  return *this;
}
