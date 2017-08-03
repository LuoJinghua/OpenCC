/*
 * Open Chinese Convert
 *
 * Copyright 2017 Luo Jinghua <sunmoon1997@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Common.hpp"
#include "DataStream.hpp"
#include "Segments.hpp"
#include "UTF8Util.hpp"

namespace opencc {
/**
 * Abstract class of file system
 * @ingroup opencc_cpp_api
 */
class OPENCC_EXPORT FileSystem {
public:
  static FileSystemPtr getFileSystem();
  static void setFileSystem(FileSystemPtr fileSystem);

  virtual DataStreamPtr
  open(const std::string& fileName,
       DataStream::AccessMode mode = DataStream::READ) = 0;

  virtual bool exists(const std::string& fileName);

private:
  static FileSystemPtr currentFileSystem;
};
}
