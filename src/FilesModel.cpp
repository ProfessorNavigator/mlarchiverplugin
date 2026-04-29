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

#include <FilesModel.h>
#include <QBrush>
#include <algorithm>
#include <iostream>

FilesModel::FilesModel(
    QObject *parent,
    const std::vector<std::tuple<std::string, uint64_t, uint64_t>> &in_archive)
    : QAbstractItemModel(parent)
{
  original = in_archive;
  for(auto it = in_archive.begin(); it != in_archive.end(); it++)
    {
      FilesModelItem item(*it);
      model.emplace_back(item);
    }
}

QModelIndex
FilesModel::index(int row, int column, const QModelIndex &parent) const
{
  QModelIndex result;

  if(parent.isValid())
    {
      return result;
    }
  if(static_cast<size_t>(row) >= model.size())
    {
      return result;
    }

  result = createIndex(row, column, model.data() + row);

  return result;
}

QModelIndex
FilesModel::parent(const QModelIndex &index) const
{
  return QModelIndex();
}

int
FilesModel::rowCount(const QModelIndex &parent) const
{
  if(parent.isValid())
    {
      return 0;
    }
  return static_cast<int>(model.size());
}

int
FilesModel::columnCount(const QModelIndex &parent) const
{
  if(parent.isValid())
    {
      return 0;
    }
  return 2;
}

QVariant
FilesModel::data(const QModelIndex &index, int role) const
{
  QVariant result;

  const FilesModelItem *item
      = reinterpret_cast<const FilesModelItem *>(index.constInternalPointer());

  if(item == nullptr)
    {
      return result;
    }

  switch(index.column())
    {
    case 0:
      {
        switch(role)
          {
          case Qt::DisplayRole:
            {
              result = QVariant(item->source_path);
              break;
            }
          case Qt::TextAlignmentRole:
            {
              result = QVariant(Qt::AlignCenter);
              break;
            }
          default:
            break;
          }
        break;
      }
    case 1:
      {
        switch(role)
          {
          case Qt::DisplayRole:
          case Qt::EditRole:
            {
              result = QVariant(item->path_in_archive);
              break;
            }
          case Qt::TextAlignmentRole:
            {
              result = QVariant(Qt::AlignCenter);
              break;
            }
          case Qt::BackgroundRole:
            {
              if(item->exists_in_archive)
                {
                  QBrush brush("red");
                  result = QVariant(brush);
                }
              break;
            }
          default:
            break;
          }

        break;
      }
    default:
      break;
    }

  return result;
}

Qt::ItemFlags
FilesModel::flags(const QModelIndex &index) const
{
  Qt::ItemFlags result = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

  switch(index.column())
    {
    case 1:
      {
        const FilesModelItem *item = reinterpret_cast<const FilesModelItem *>(
            index.constInternalPointer());
        if(item == nullptr)
          {
            break;
          }
        if(item->source_path.isEmpty())
          {
            break;
          }
        result |= Qt::ItemIsEditable;
        break;
      }
    default:
      break;
    }

  return result;
}

QVariant
FilesModel::headerData(int section, Qt::Orientation orientation,
                       int role) const
{
  QVariant result;

  switch(orientation)
    {
    case Qt::Orientation::Horizontal:
      {
        switch(section)
          {
          case 0:
            {
              switch(role)
                {
                case Qt::DisplayRole:
                  {
                    QString str(tr("Source file path"));
                    result = QVariant(str);
                    break;
                  }
                case Qt::TextAlignmentRole:
                  {
                    result = QVariant(Qt::AlignCenter);
                    break;
                  }
                default:
                  break;
                }
              break;
            }
          case 1:
            {
              switch(role)
                {
                case Qt::DisplayRole:
                  {
                    QString str(tr("Path in archive"));
                    result = QVariant(str);
                    break;
                  }
                case Qt::TextAlignmentRole:
                  {
                    result = QVariant(Qt::AlignCenter);
                    break;
                  }
                default:
                  break;
                }
              break;
            }
          default:
            break;
          }
        break;
      }
    case Qt::Orientation::Vertical:
      {
        switch(role)
          {
          case Qt::DisplayRole:
            {
              QString str;
              str.setNum(section + 1);
              result = QVariant(str);
              break;
            }
          case Qt::TextAlignmentRole:
            {
              result = QVariant(Qt::AlignCenter);
              break;
            }
          default:
            break;
          }
        break;
      }
    default:
      break;
    }

  return result;
}

bool
FilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  bool result = false;

  if(index.column() != 1)
    {
      return result;
    }

  if(role != Qt::EditRole)
    {
      return result;
    }

  FilesModelItem *item = const_cast<FilesModelItem *>(
      reinterpret_cast<const FilesModelItem *>(index.constInternalPointer()));
  item->path_in_archive = value.toString();

#ifdef _WIN32
  QString before("\\");
  QString after("/");
  item->path_in_archive.replace(before, after);
#endif

  item->exists_in_archive = false;
  for(FilesModelItem *i = model.data(); i != model.data() + model.size(); i++)
    {
      if(i == item)
        {
          continue;
        }
      if(i->path_in_archive == item->path_in_archive)
        {
          item->exists_in_archive = true;
          break;
        }
    }

  result = true;
  emit dataChanged(index, index);

  return result;
}

void
FilesModel::addFiles(const QStringList &files)
{
  std::vector<FilesModelItem> fl;
  for(qsizetype i = 0; i < files.size(); i++)
    {
      std::string str = files[i].toStdString();
      std::filesystem::path src = std::u8string(str.begin(), str.end());
      auto it = std::find_if(model.begin(), model.end(),
                             [src](const FilesModelItem &el)
                               {
                                 std::string str
                                     = el.source_path.toStdString();
                                 std::filesystem::path p
                                     = std::u8string(str.begin(), str.end());
                                 return p == src;
                               });
      if(it != model.end())
        {
          continue;
        }

      FilesModelItem item(src);
      it = std::find_if(model.begin(), model.end(),
                        [item](const FilesModelItem &el)
                          {
                            return item.path_in_archive == el.path_in_archive;
                          });
      if(it != model.end())
        {
          item.exists_in_archive = true;
        }
      fl.emplace_back(item);
    }

  if(fl.size() == 0)
    {
      return void();
    }

  beginInsertRows(QModelIndex(), static_cast<int>(model.size()),
                  static_cast<int>(model.size() + fl.size() - 1));
  std::copy(fl.begin(), fl.end(), std::back_inserter(model));
  endInsertRows();
}

void
FilesModel::addDirectory(const QString &path)
{
  std::string str = path.toStdString();
  std::filesystem::path dir_path = std::u8string(str.begin(), str.end());
  std::vector<FilesModelItem> fl;
  std::error_code ec;
  std::filesystem::path base_p = dir_path.parent_path();
  for(auto &dir_it : std::filesystem::recursive_directory_iterator(
          dir_path,
          std::filesystem::directory_options::follow_directory_symlink
              | std::filesystem::directory_options::skip_permission_denied,
          ec))
    {
      std::filesystem::path p = dir_it.path();
      std::error_code l_ec;
      std::filesystem::file_status stat
          = std::filesystem::symlink_status(p, l_ec);
      if(l_ec)
        {
          std::cout << "FilesModel::addDirectory: \"" << l_ec.message() << "\""
                    << std::endl;
          continue;
        }
      switch(stat.type())
        {
        case std::filesystem::file_type::symlink:
          {
            std::filesystem::path lp = std::filesystem::read_symlink(p, l_ec);
            if(l_ec)
              {
                std::cout << "FilesModel::addDirectory: \"" << l_ec.message()
                          << "\" " << p << std::endl;
                continue;
              }
            if(std::filesystem::is_directory(lp))
              {
                continue;
              }
            p = lp;
          }
        case std::filesystem::file_type::regular:
          {
            break;
          }
        default:
          {
            continue;
            break;
          }
        }
      std::filesystem::path rel = p.lexically_relative(base_p);
      std::u8string u8str = rel.u8string();
#ifdef _WIN32
      std::replace(u8str.begin(), u8str.end(), '\\', '/');
#endif
      std::u8string find_str(u8"../");
      bool found = false;
      for(;;)
        {
          std::u8string::size_type n = u8str.find(find_str);
          if(n != std::u8string::npos)
            {
              found = true;
              u8str.erase(0, n + find_str.size());
            }
          else
            {
              break;
            }
        }
      if(found)
        {
          u8str = dir_path.filename().u8string() + u8"/" + u8str;
        }

      FilesModelItem item;
      item.source_path = p.u8string().c_str();
      item.path_in_archive = u8str.c_str();
      item.source_size
          = static_cast<uint64_t>(std::filesystem::file_size(p, l_ec));
      if(l_ec)
        {
          std::cout << "FilesModel::addDirectory: \"" << l_ec.message()
                    << "\" " << p << std::endl;
        }

      auto it_m = std::find_if(model.begin(), model.end(),
                               [item](const FilesModelItem &el)
                                 {
                                   return el.source_path == item.source_path;
                                 });
      if(it_m != model.end())
        {
          continue;
        }

      it_m
          = std::find_if(model.begin(), model.end(),
                         [item](const FilesModelItem &el)
                           {
                             return el.path_in_archive == item.path_in_archive;
                           });
      if(it_m != model.end())
        {
          item.exists_in_archive = true;
        }
      fl.emplace_back(item);
    }
  if(ec)
    {
      std::cout << "FilesModel::addDirectory: \"" << ec.message() << "\""
                << std::endl;
      return void();
    }

  if(fl.size() == 0)
    {
      return void();
    }
  beginInsertRows(QModelIndex(), static_cast<int>(model.size()),
                  static_cast<int>(model.size() + fl.size() - 1));
  std::copy(fl.begin(), fl.end(), std::back_inserter(model));
  endInsertRows();
}

void
FilesModel::removeEntry(const QModelIndex &index)
{
  const FilesModelItem *item
      = reinterpret_cast<const FilesModelItem *>(index.constInternalPointer());
  if(item == nullptr)
    {
      return void();
    }
  QString arch_nm = item->path_in_archive;
  beginResetModel();
  model.erase(model.begin() + index.row());
  auto it = std::find_if(model.begin(), model.end(),
                         [arch_nm](const FilesModelItem &el)
                           {
                             return el.path_in_archive == arch_nm;
                           });
  if(it != model.end())
    {
      it->exists_in_archive = false;
    }
  endResetModel();
}

void
FilesModel::resetToDefault()
{
  beginResetModel();
  model.clear();
  for(auto it = original.begin(); it != original.end(); it++)
    {
      FilesModelItem item(*it);
      model.emplace_back(item);
    }
  endResetModel();
}

std::vector<FilesModelItem>
FilesModel::getModel()
{
  return model;
}
