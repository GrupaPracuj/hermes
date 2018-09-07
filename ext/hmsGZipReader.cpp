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
    
    std::unique_ptr<DataStorageReader> GZipLoader::createStorageReader(DataReader& pReader, const std::string& pPrefix) const
    {
        return std::unique_ptr<GZipReader>(new GZipReader(pReader, pPrefix));
    }
    
    GZipReader::GZipReader(const std::string& pPath, const std::string& pPrefix) : DataStorageReader(pPrefix)
    {
        mBigEndian = !tools::isLittleEndian();
        
        FileReader reader{pPath};
        getCompressedData(reader, pPath);
    }
    
    GZipReader::GZipReader(DataReader& pReader, const std::string& pPrefix) : DataStorageReader(pPrefix)
    {
        mBigEndian = !tools::isLittleEndian();
        getCompressedData(pReader);
    }

    std::unique_ptr<DataReader> GZipReader::openFile(const std::string& pName) const
    {
        std::unique_ptr<MemoryReader> reader = nullptr;
        if (mCompressedData.size() != 0 && mUncompressedSize != 0 && mPrefix + mFileName == pName)
        {
            char* output = new char[mUncompressedSize];
            z_stream stream;
            
            stream.next_in = (z_const Bytef*)mCompressedData.data();
            stream.avail_in = static_cast<uInt>(mCompressedData.size());
            stream.next_out = (Bytef*)output;
            stream.avail_out = static_cast<uInt>(mUncompressedSize);
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
                reader = std::make_unique<MemoryReader>(output, mUncompressedSize, true);
            }
        }
        
        return reader;
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
    
    void GZipReader::getCompressedData(DataReader& pReader, const std::string& pFilePath)
    {
        pReader.seek(0);
        
        if (pReader.read(&mHeader, sizeof(mHeader)) == sizeof(mHeader))
        {
            if (mBigEndian)
            {
                mHeader.mSignature = tools::byteSwap16(mHeader.mSignature);
                mHeader.mModificationTime = tools::byteSwap32(mHeader.mModificationTime);
            }
            
            if (mHeader.mSignature != 0x8b1f)
                return;
            
            const uint8_t crc16 = 1 << 1;
            const uint8_t extra = 1 << 2;
            const uint8_t fileName = 1 << 3;
            const uint8_t comment = 1 << 4;
            
            if ((mHeader.mFlags & extra) == extra)
            {
                uint16_t length = 0;
                pReader.read(&length, 2);
                
                if (mBigEndian)
                    length = tools::byteSwap16(length);
                
                pReader.seek(static_cast<int64_t>(length), true);
            }
            
            if ((mHeader.mFlags & fileName) == fileName)
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
            
            if ((mHeader.mFlags & comment) == comment)
            {
                char c = 0;
                do
                    pReader.read(&c, 1);
                while (c != 0);
            }
            
            if ((mHeader.mFlags & crc16) == crc16)
                pReader.seek(2, true);
            
            pReader.read(mCompressedData, pReader.getSize() - 8 - pReader.getPosition());
            
            uint32_t crc = 0;
            pReader.read(&crc, 4);
            pReader.read(&mUncompressedSize, 4);
            
            if (mBigEndian)
                mUncompressedSize = tools::byteSwap32(mUncompressedSize);
        }
        
        pReader.seek(0);
    }
}
}
        
