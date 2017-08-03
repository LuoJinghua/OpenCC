
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
#include "FileSystem.hpp"
#include "DataStream.hpp"

namespace opencc {

class DefaultFileSystem : public FileSystem {
public:
  DataStreamPtr open(const std::string& fileName, DataStream::AccessMode mode) {
#ifdef _MSC_VER
    const wchar_t* open_mode = mode == DataStream::READ ? L"rb" : L"wb";
    // well, the 'GetPlatformString' shall return a 'wstring'
    FILE* fp =
        _wfopen(UTF8Util::GetPlatformString(fileName).c_str(), open_mode);
#else
    const char* open_mode = mode == DataStream::READ ? "rb" : "wb";
    FILE* fp = fopen(UTF8Util::GetPlatformString(fileName).c_str(), open_mode);
#endif // _MSC_VER
    if (!fp)
      return DataStreamPtr();
    return DataStreamPtr(new FileHandleDataStream(fp));
  }
};

FileSystemPtr FileSystem::currentFileSystem(new DefaultFileSystem());

FileSystemPtr FileSystem::getFileSystem() { return currentFileSystem; }

void FileSystem::setFileSystem(FileSystemPtr fileSystem) {
  currentFileSystem = fileSystem;
}

bool FileSystem::exists(const std::string& fileName) {
  DataStreamPtr ds = open(fileName);
  if (!ds)
    return false;
  ds->close();
  return true;
}
}
