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
        bool loadable = true;
        
        if ((loadable = pPath.length() > extension.length()))
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
                        DataBuffer compressedData;
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
                        
                        if (mBigEndian)
                            uncompressedSize = tools::byteSwap32(uncompressedSize);
                        
                        if (compressedData.size() != 0 && uncompressedSize != 0)
                        {
                            char* output = new char[uncompressedSize];
                            z_stream stream;
                            
                            stream.next_in = (z_const Bytef*)compressedData.data();
                            stream.avail_in = static_cast<uInt>(compressedData.size());
                            stream.next_out = (Bytef*)output;
                            stream.avail_out = static_cast<uInt>(uncompressedSize);
                            stream.zalloc = (alloc_func)0;
                            stream.zfree = (free_func)0;
                            
                            int32_t error = inflateInit2(&stream, -MAX_WBITS);
                            if (error == Z_OK)
                            {
                                error = inflate(&stream, Z_FINISH);
                                inflateEnd(&stream);
                            }
                            
                            if (error != Z_STREAM_END)
                            {
                                Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Gzip decompress error %", error);
                                delete[] output;
                            }
                            else
                            {
                                memory = std::make_unique<MemoryReader>(output, uncompressedSize, true);
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
        