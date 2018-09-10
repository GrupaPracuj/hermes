#ifndef _HMS_GZIP_READER_HPP_
#define _HMS_GZIP_READER_HPP_

#include "hmsData.hpp"

namespace hms
{
namespace ext
{
    class GZipLoader : public DataStorageLoader
    {
    public:
        GZipLoader();
        virtual ~GZipLoader() = default;
        
        virtual bool isLoadable(const std::string& pPath) const override;
        virtual bool isLoadable(DataReader& pReader) const override;
        virtual std::unique_ptr<DataStorageReader> createStorageReader(const std::string& pPath, const std::string& pPrefix = "") const override;
        virtual std::unique_ptr<DataStorageReader> createStorageReader(std::shared_ptr<DataReader> pReader, const std::string& pPrefix = "") const override;
        
    private:
        bool mBigEndian = false;
        
    };
    
    class GZipReader : public DataStorageReader
    {
    public:
        GZipReader(const std::string& pPath, const std::string& pPrefix);
        GZipReader(std::shared_ptr<DataReader> pReader, const std::string& pPrefix);
        virtual ~GZipReader() = default;
        
        virtual std::unique_ptr<DataReader> openFile(const std::string& pName) const override;
        virtual bool clearStorage(const std::string& pPath) override;
        
        const std::string& getFileName() const;
        void setFileName(std::string pFileName);
        
    private:
        struct GZipHeader
        {
            uint16_t mSignature; // 0x8b1f
            uint8_t  mCompressionMethod; // 8
            uint8_t  mFlags;
            uint32_t mModificationTime;
            uint8_t  mFlagsExtra;
            uint8_t  mOS;
        } __attribute__((packed));
        
        std::shared_ptr<DataReader> mReader = nullptr;
        std::string mFilePath;
        std::string mFileName;
        bool mBigEndian = false;
        
        void unpackFileName(DataReader& pReader, const std::string& pFilePath = "");
    };
}
}

#endif
