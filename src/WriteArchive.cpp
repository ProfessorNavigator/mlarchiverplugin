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

#include <WriteArchive.h>
#include <archive.h>
#include <archive_entry.h>
#include <iostream>

WriteArchive::WriteArchive(const std::shared_ptr<MLBookProc> &mlbp,
                           const std::filesystem::path &archive_path,
                           const std::vector<FilesModelItem> &files,
                           const bool &overwrite)
    : LibArchive(mlbp)
{
  this->archive_path = archive_path;
  this->files = files;
  this->overwrite = overwrite;
  for(auto it = files.begin(); it != files.end(); it++)
    {
      total += static_cast<double>(it->source_size);
    }
  cancel.store(false, std::memory_order_relaxed);
}

void
WriteArchive::writeArchive()
{
  tmp = archive_path.parent_path();  
  tmp /= mlbp->randomFileName();
  tmp.replace_extension(archive_path.extension());

  while(std::filesystem::exists(tmp))
    {
      tmp = archive_path.parent_path();
      tmp /= mlbp->randomFileName();
      tmp.replace_extension(archive_path.extension());
    }

  std::shared_ptr<LibArchiveFileData> fd_write(new LibArchiveFileData);
  fd_write->path = tmp;
  fd_write->open_mode = std::ios_base::out | std::ios_base::binary;

  a_write = initForWriting(fd_write);

  int er = archive_write_set_format_filter_by_ext(
      a_write.get(), archive_path.string().c_str());
  if(er != ARCHIVE_OK)
    {
      archiveError(a_write, "WriteArchive::writeArchive:");
    }

  er = archive_write_set_options(a_write.get(), "hdrcharset=UTF-8");
  if(er != ARCHIVE_OK)
    {
      try
        {
          archiveError(a_write, "WriteArchive::writeArchive:");
        }
      catch(std::exception &ex)
        {
          std::cout << ex.what() << std::endl;
        }
    }

  er = archive_write_open(
      a_write.get(), fd_write.get(), &LibArchive::openCallBack,
      &LibArchive::writeCallback, &LibArchive::closeCallback);
  if(er != ARCHIVE_OK)
    {
      archiveError(a_write, "WriteArchive::writeArchive:");
    }
  if(overwrite)
    {
      try
        {
          processOriginalArchive();
        }
      catch(std::exception &er)
        {
          std::cout << er.what() << std::endl;
        }
    }

  for(auto it = files.begin(); it != files.end(); it++)
    {
      if(cancel.load(std::memory_order_relaxed))
        {
          break;
        }
      if(it->source_path.isEmpty())
        {
          continue;
        }
      std::string str = it->source_path.toStdString();
      std::filesystem::path p = std::u8string(str.begin(), str.end());
      str = it->path_in_archive.toStdString();
      writeFile(a_write, p, str, std::filesystem::perms::none);

      progress += static_cast<double>(it->source_size);
      if(signal_progress)
        {
          signal_progress(progress, total);
        }
    }

  a_write.reset();
  fd_write.reset();

  if(cancel.load(std::memory_order_relaxed))
    {
      std::filesystem::remove_all(tmp);
    }
  else
    {
      std::filesystem::remove_all(archive_path);
      std::filesystem::rename(tmp, archive_path);
    }
}

void
WriteArchive::cancelAll()
{
  cancel.store(true, std::memory_order_relaxed);
}

bool
WriteArchive::getCanceled()
{
  return cancel.load(std::memory_order_relaxed);
}

void
WriteArchive::processOriginalArchive()
{
  std::shared_ptr<LibArchiveFileData> fd_read(new LibArchiveFileData);
  fd_read->path = archive_path;
  fd_read->open_mode = std::ios_base::in | std::ios_base::binary;

  std::shared_ptr<archive> a_read = initForReading(fd_read);

  int er = archive_read_set_seek_callback(a_read.get(),
                                          &LibArchive::seekCallback);
  if(er != ARCHIVE_OK)
    {
      archiveError(a_read, "WriteArchive::processOriginalArchive:");
    }

  er = archive_read_open2(a_read.get(), fd_read.get(),
                          &LibArchive::openCallBack, &LibArchive::readCallBack,
                          &LibArchive::skipCallback,
                          &LibArchive::closeCallback);

  if(er != ARCHIVE_OK)
    {
      archiveError(a_read, "WriteArchive::processOriginalArchive:");
    }

  std::shared_ptr<archive_entry> e(archive_entry_new(),
                                   [](archive_entry *e)
                                     {
                                       archive_entry_free(e);
                                     });
  int retry_count = 0;
  while(er >= ARCHIVE_WARN && er <= ARCHIVE_OK && retry_count < 3)
    {
      if(cancel.load(std::memory_order_relaxed))
        {
          break;
        }
      archive_entry_clear(e.get());
      er = archive_read_next_header2(a_read.get(), e.get());
      switch(er)
        {
        case ARCHIVE_WARN:
          {
            const char *str = archive_error_string(a_read.get());
            std::string err;
            if(str)
              {
                err = std::string("WriteArchive::processOriginalArchive: \"")
                      + str + "\"";
              }
            else
              {
                err = std::string("WriteArchive::processOriginalArchive: ")
                      + std::strerror(archive_errno(a_read.get()));
              }
            std::cout << err << std::endl;
          }
        case ARCHIVE_OK:
          {
            retry_count = 0;
            const char *val = archive_entry_pathname_utf8(e.get());
            if(val)
              {
                std::string p_in_arch(val);
                QString str = p_in_arch.c_str();
                auto it = std::find_if(files.begin(), files.end(),
                                       [str](const FilesModelItem &el)
                                         {
                                           return el.path_in_archive == str;
                                         });
                if(it == files.end())
                  {
                    continue;
                  }
                else
                  {
                    if(!it->source_path.isEmpty())
                      {
                        continue;
                      }
                  }
                if(archive_entry_size_is_set(e.get()))
                  {
                    progress
                        += static_cast<double>(archive_entry_size(e.get()));
                  }
                if(signal_progress)
                  {
                    signal_progress(progress, total);
                  }
              }

            std::string buf = unpackEntryToBuffer(a_read, e);
            setUsernameGroupname(e);
            if(buf.size() > 0)
              {
                writeBufferToArchive(a_write, e, buf);
              }
            else
              {
                er = archive_write_header(a_write.get(), e.get());
                if(er != ARCHIVE_OK)
                  {
                    archiveError(a_write,
                                 "WriteArchive::processOriginalArchive:");
                  }
              }
            break;
          }
        case ARCHIVE_EOF:
          {
            break;
          }
        case ARCHIVE_RETRY:
          {
            retry_count++;
            break;
          }
        default:
          {
            archiveError(a_read, "WriteArchive::processOriginalArchive:");
            break;
          }
        }
    }
}
