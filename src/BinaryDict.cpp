/*
 * Open Chinese Convert
 *
 * Copyright 2010-2014 BYVoid <byvoid@byvoid.com>
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

#include "BinaryDict.hpp"
#include "Lexicon.hpp"

using namespace opencc;

size_t BinaryDict::KeyMaxLength() const {
  size_t maxLength = 0;
  for (const DictEntry* entry : *lexicon) {
    maxLength = (std::max)(maxLength, entry->KeyLength());
  }
  return maxLength;
}

void BinaryDict::SerializeToFile(FILE* fp) const {
  string keyBuf, valueBuf;
  vector<size_t> keyOffsets, valueOffsets;
  unsigned keyTotalLength = 0, valueTotalLength = 0;
  ConstructBuffer(keyBuf, keyOffsets, keyTotalLength, valueBuf,
                  valueOffsets, valueTotalLength);
  // Number of items
  unsigned numItems = static_cast<unsigned>(lexicon->Length());
  fwrite(&numItems, sizeof(unsigned), 1, fp);

  // Data
  fwrite(&keyTotalLength, sizeof(unsigned), 1, fp);
  fwrite(keyBuf.c_str(), sizeof(char), keyTotalLength, fp);
  fwrite(&valueTotalLength, sizeof(unsigned), 1, fp);
  fwrite(valueBuf.c_str(), sizeof(char), valueTotalLength, fp);

  size_t keyCursor = 0, valueCursor = 0;
  for (const DictEntry* entry : *lexicon) {
    // Number of values
    unsigned numValues = static_cast<unsigned>(entry->NumValues());
    fwrite(&numValues, sizeof(unsigned), 1, fp);
    // Key offset
    unsigned keyOffset = static_cast<unsigned>(keyOffsets[keyCursor++]);
    fwrite(&keyOffset, sizeof(unsigned), 1, fp);
    // Values offset
    for (size_t i = 0; i < numValues; i++) {
      unsigned valueOffset = static_cast<unsigned>(valueOffsets[valueCursor++]);
      fwrite(&valueOffset, sizeof(unsigned), 1, fp);
    }
  }
  assert(keyCursor == numItems);
}

BinaryDictPtr BinaryDict::NewFromFile(DataStreamPtr fp) {
  BinaryDictPtr dict(new BinaryDict(LexiconPtr(new Lexicon)));

  // Number of items
  unsigned numItems;
  size_t unitsRead = fp->read(&numItems, sizeof(unsigned));
  if (unitsRead != sizeof(unsigned)) {
    throw InvalidFormat("Invalid OpenCC binary dictionary (numItems)");
  }

  // Keys
  unsigned keyTotalLength;
  unitsRead = fp->read(&keyTotalLength, sizeof(unsigned));
  if (unitsRead != sizeof(unsigned)) {
    throw InvalidFormat("Invalid OpenCC binary dictionary (keyTotalLength)");
  }
  dict->keyBuffer.resize(keyTotalLength);
  unitsRead = fp->read(const_cast<char*>(dict->keyBuffer.c_str()),
                    keyTotalLength);
  if (unitsRead != keyTotalLength) {
    throw InvalidFormat("Invalid OpenCC binary dictionary (keyBuffer)");
  }

  // Values
  unsigned valueTotalLength;
  unitsRead = fp->read(&valueTotalLength, sizeof(unsigned));
  if (unitsRead != sizeof(unsigned)) {
    throw InvalidFormat("Invalid OpenCC binary dictionary (valueTotalLength)");
  }
  dict->valueBuffer.resize(valueTotalLength);
  unitsRead = fp->read(const_cast<char*>(dict->valueBuffer.c_str()),
                    valueTotalLength);
  if (unitsRead != valueTotalLength) {
    throw InvalidFormat("Invalid OpenCC binary dictionary (valueBuffer)");
  }

  // Offsets
  for (size_t i = 0; i < numItems; i++) {
    // Number of values
    unsigned numValues;
    unitsRead = fp->read(&numValues, sizeof(unsigned));
    if (unitsRead != sizeof(unsigned)) {
      throw InvalidFormat("Invalid OpenCC binary dictionary (numValues)");
    }
    // Key offset
    unsigned keyOffset;
    unitsRead = fp->read(&keyOffset, sizeof(unsigned));
    if (unitsRead != sizeof(unsigned)) {
      throw InvalidFormat("Invalid OpenCC binary dictionary (keyOffset)");
    }
    const char* key = dict->keyBuffer.c_str() + keyOffset;
    // Value offset
    vector<const char*> values;
    for (size_t j = 0; j < numValues; j++) {
      unsigned valueOffset;
      unitsRead = fp->read(&valueOffset, sizeof(unsigned));
      if (unitsRead != sizeof(unsigned)) {
        throw InvalidFormat("Invalid OpenCC binary dictionary (valueOffset)");
      }
      const char* value = dict->valueBuffer.c_str() + valueOffset;
      values.push_back(value);
    }
    PtrDictEntry* entry = new PtrDictEntry(key, values);
    dict->lexicon->Add(entry);
  }

  return dict;
}

void BinaryDict::ConstructBuffer(string& keyBuf, vector<size_t>& keyOffset,
                                 unsigned& keyTotalLength, string& valueBuf,
                                 vector<size_t>& valueOffset,
                                 unsigned& valueTotalLength) const {
  keyTotalLength = 0;
  valueTotalLength = 0;
  // Calculate total length
  for (const DictEntry* entry : *lexicon) {
    keyTotalLength += entry->KeyLength() + 1;
    assert(entry->NumValues() != 0);
    if (entry->NumValues() == 1) {
      const auto* svEntry = static_cast<const SingleValueDictEntry*>(entry);
      valueTotalLength += strlen(svEntry->Value()) + 1;
    } else {
      const auto* mvEntry = static_cast<const MultiValueDictEntry*>(entry);
      for (const auto& value : mvEntry->Values()) {
        valueTotalLength += strlen(value) + 1;
      }
    }
  }
  // Write keys and values to buffers
  keyBuf.resize(keyTotalLength, '\0');
  valueBuf.resize(valueTotalLength, '\0');
  char* pKeyBuffer = const_cast<char*>(keyBuf.c_str());
  char* pValueBuffer = const_cast<char*>(valueBuf.c_str());
  for (const DictEntry* entry : *lexicon) {
    strcpy(pKeyBuffer, entry->Key());
    keyOffset.push_back(pKeyBuffer - keyBuf.c_str());
    pKeyBuffer += entry->KeyLength() + 1;
    if (entry->NumValues() == 1) {
      const auto* svEntry = static_cast<const SingleValueDictEntry*>(entry);
      strcpy(pValueBuffer, svEntry->Value());
      valueOffset.push_back(pValueBuffer - valueBuf.c_str());
      pValueBuffer += strlen(svEntry->Value()) + 1;
    } else {
      const auto* mvEntry = static_cast<const MultiValueDictEntry*>(entry);
      for (const auto& value : mvEntry->Values()) {
        strcpy(pValueBuffer, value);
        valueOffset.push_back(pValueBuffer - valueBuf.c_str());
        pValueBuffer += strlen(value) + 1;
      }
    }
  }
  assert(keyBuf.c_str() + keyTotalLength == pKeyBuffer);
  assert(valueBuf.c_str() + valueTotalLength == pValueBuffer);
}
