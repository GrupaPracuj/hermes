// Copyright (C) 2017-2023 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsGZipReader.hpp"

#include "hermes.hpp"
#include "hmsTools.hpp"
#include "zlib.h"

namespace hms
{
namespace ext
{
    GZipLoader::GZipLoader()
    {
        mBigEndian = !tools::isLittleEndian();
    }
    
    bool GZipLoader::isLoadable(const std::string& pPath) const
    {
        const std::string extension = ".gz";
        bool loadable = pPath.length() > extension.length();
        
        if (loadable)
        {
            auto pIt = pPath.crbegin();
            auto eIt = extension.crbegin();
            for (; pIt != pPath.crend() && eIt != extension.crend(); pIt++, eIt++)
            {
                if (*pIt != *eIt)
                {
                    loadable = false;
                    break;
                }
            }
        }
        
        return loadable;
    }
    
    bool GZipLoader::isLoadable(DataReader& pReader) const
    {
        pReader.seek(0);

        uint16_t signature;
        pReader.read(&signature, sizeof(signature));

        if (mBigEndian)
            signature = tools::byteSwap16(signature);
    
        pReader.seek(0);

        return signature == 0x8b1f;
    }
    
    std::unique_ptr<DataStorageReader> GZipLoader::createStorageReader(const std::string& pPath, const std::string& pPrefix) const
    {
        return std::unique_ptr<GZipReader>(new GZipReader(pPath, pPrefix));
    }
    
    std::unique_ptr<DataStorageReader> GZipLoader::createStorageReader(std::shared_ptr<DataReader> pReader, const std::string& pPrefix) const
    {
        return std::unique_ptr<GZipReader>(new GZipReader(pReader, pPrefix));
    }
    
    GZipReader::GZipReader(const std::string& pPath, const std::string& pPrefix) : DataStorageReader(pPrefix), mFilePath(std::move(pPath))
    {
        mBigEndian = !tools::isLittleEndian();
        
        FileReader reader{mFilePath};
        unpackFileName(reader, mFilePath);
    }
    
    GZipReader::GZipReader(std::shared_ptr<DataReader> pReader, const std::string& pPrefix) : DataStorageReader(pPrefix), mReader(pReader)
    {
        mBigEndian = !tools::isLittleEndian();
        unpackFileName(*pReader);
    }

    std::unique_ptr<DataReader> GZipReader::openFile(const std::string& pName) const
    {
        std::unique_ptr<MemoryReader> memory = nullptr;
        
        if (mPrefix + mFileName == pName)
        {
            FileReader file{mFilePath};
            DataReader* reader = mReader != nullptr ? mReader.get() : (file.isOpen() ? &file : nullptr);
            
            if (reader != nullptr)
            {
                GZipHeader header;
                
                reader->seek(0);
                if (reader->read(&header, sizeof(header)) == sizeof(header))
                {
                    if (mBigEndian)
                    {
                        header.mSignature = tools::byteSwap16(header.mSignature);
                        header.mModificationTime = tools::byteSwap32(header.mModificationTime);
                    }
                    
                    if (header.mSignature == 0x8b1f)
                    {
                        const uint8_t crc16 = 1 << 1;
                        const uint8_t extra = 1 << 2;
                        const uint8_t fileName = 1 << 3;
                        const uint8_t comment = 1 << 4;
                        std::vector<uint8_t> compressedData;
                        uint32_t crc = 0;
                        uint32_t uncompressedSize = 0;
                        auto readUntilZero = [](DataReader& lpReader) -> void
                        {
                            char c = 0;
                            do
                                lpReader.read(&c, 1);
                            while (c != 0);
                        };
                        
                        if ((header.mFlags & extra) == extra)
                        {
                            uint16_t length = 0;
                            reader->read(&length, 2);
                            
                            if (mBigEndian)
                                length = tools::byteSwap16(length);
                            
                            reader->seek(static_cast<int64_t>(length), true);
                        }
                        
                        if ((header.mFlags & fileName) == fileName)
                            readUntilZero(*reader);
                        
                        if ((header.mFlags & comment) == comment)
                            readUntilZero(*reader);
                        
                        if ((header.mFlags & crc16) == crc16)
                            reader->seek(2, true);
                        
                        reader->read(compressedData, reader->getSize() - 8 - reader->getPosition());
                        reader->read(&crc, 4);
                        reader->read(&uncompressedSize, 4);

                        if (compressedData.size() != 0)
                        {
                            const size_t chunkSize = 1024 * 256;
                            Bytef chunk[chunkSize];
                            std::vector<uint8_t> output;
                            
                            z_stream stream;
                            stream.zalloc = Z_NULL;
                            stream.zfree = Z_NULL;
                            stream.opaque = Z_NULL;
                            stream.next_in = (z_const Bytef*)compressedData.data();
                            stream.avail_in = static_cast<uInt>(compressedData.size());
                            
                            int32_t error = inflateInit2(&stream, -MAX_WBITS);
                            if (error == Z_OK)
                            {
                                do
                                {
                                    stream.avail_out = chunkSize;
                                    stream.next_out = chunk;
                                    error = inflate(&stream, Z_NO_FLUSH);
                                    
                                    if (!(error == Z_OK || error == Z_STREAM_END))
                                        break;

                                    output.insert(output.end(), chunk, chunk + (chunkSize - stream.avail_out));
                                }
                                while (stream.avail_out == 0 && error != Z_STREAM_END);

                                inflateEnd(&stream);
                            }
                            
                            if (error != Z_STREAM_END)
                            {
                                Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "gzip decompress error %", error);
                            }
                            else
                            {
                                const size_t outputSize = output.size();
                                Bytef* outputMemory = new Bytef[outputSize];
                                memcpy(outputMemory, output.data(), outputSize);
                                memory = std::make_unique<MemoryReader>(outputMemory, outputSize, true);
                            }
                        }
                    }
                    reader->seek(0);
                }
            }
        }
        
        return memory;
    }

    bool GZipReader::clearStorage(const std::string& pPath)
    {
        return false;
    };

    const std::string& GZipReader::getFileName() const
    {
        return mFileName;
    }
    
    void GZipReader::setFileName(std::string pFileName)
    {
        mFileName = std::move(pFileName);
    }
    
    void GZipReader::unpackFileName(DataReader& pReader, const std::string& pFilePath)
    {
        GZipHeader header;
        
        pReader.seek(0);
        if (pReader.read(&header, sizeof(header)) == sizeof(header))
        {
            if (mBigEndian)
                header.mSignature = tools::byteSwap16(header.mSignature);
            
            if (header.mSignature != 0x8b1f)
                return;
            
            const uint8_t extra = 1 << 2;
            const uint8_t fileName = 1 << 3;
            
            if ((header.mFlags & extra) == extra)
            {
                uint16_t length = 0;
                pReader.read(&length, 2);
                
                if (mBigEndian)
                    length = tools::byteSwap16(length);
                
                pReader.seek(static_cast<int64_t>(length), true);
            }
            
            if ((header.mFlags & fileName) == fileName)
            {
                const size_t length = 1024;
                char name[length] = {0};
                size_t i = 0;
                
                do
                    pReader.read(name + i++, 1);
                while (name[i - 1] != 0 && i < length);
                
                name[length - 1] = 0;
                mFileName = name;
            }
            else if (pFilePath.length() != 0)
            {
                const size_t dotPos = pFilePath.rfind('.');
                const size_t slashPos = pFilePath.rfind('/');
                const size_t begin = slashPos < pFilePath.length() ? slashPos + 1 : 0;
                const size_t length = dotPos < pFilePath.length() && slashPos < dotPos ? dotPos - (slashPos + 1) : std::string::npos;
                mFileName = pFilePath.substr(begin, length);
            }
            
            pReader.seek(0);
        }
    }
}
}
        
