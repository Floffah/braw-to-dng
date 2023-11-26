#include "BlackmagicRawAPI.h"
#include "BlackmagicRawAPIDispatch.cpp"

#include <iostream>
#include <sys/stat.h>
#include <sys/dir.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

#ifdef DEBUG
    #include <cassert>
    #define VERIFY(condition) assert(SUCCEEDED(condition))
#else
    #define VERIFY(condition) condition
#endif

BlackmagicRawResourceFormat s_resourceFormat = blackmagicRawResourceFormatRGBAU8;

static const CFStringRef s_brawSdkPath = CFSTR("/Applications/Blackmagic RAW/Blackmagic RAW SDK/Mac/Libraries");
CFStringRef s_outputFileName = CFSTR("/tmp/outputRGBAU8.png");

static void OutputImage(CFStringRef outputFileName, uint32_t width, uint32_t height, uint32_t sizeBytes, void* imageData)
{
    bool success = false;
    const char* outputFileNameAsCString = CFStringGetCStringPtr(outputFileName, kCFStringEncodingMacRoman);

    CFURLRef file = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, outputFileName, kCFURLPOSIXPathStyle, false);
    if (file != nullptr)
    {
        const uint32_t bitsPerComponent	= 8;
        const uint32_t bitsPerPixel		= 32;
        const uint32_t bytesPerRow		= (bitsPerPixel * width) / 8U;

        CGColorSpaceRef space			= CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        CGBitmapInfo bitmapInfo			= kCGImageAlphaNoneSkipLast | kCGImageByteOrderDefault;
        CGDataProviderRef provider		= CGDataProviderCreateWithData(nullptr, imageData, sizeBytes, nullptr);
        const CGFloat* decode			= nullptr;
        bool shouldInterpolate			= false;
        CGColorRenderingIntent intent	= kCGRenderingIntentDefault;

        CGImageRef imageRef = CGImageCreate(width, height, bitsPerComponent, bitsPerPixel, bytesPerRow, space, bitmapInfo, provider, decode, shouldInterpolate, intent);
        if (imageRef != nullptr)
        {
            CGImageDestinationRef destination = CGImageDestinationCreateWithURL(file, kUTTypePNG, 1, nullptr);
            if (destination)
            {
                CGImageDestinationAddImage(destination, imageRef, nil);
                CGImageDestinationFinalize(destination);

                CFRelease(destination);

                std::cout << "Created " << outputFileNameAsCString << std::endl;
                success = true;
            }

            CGImageRelease(imageRef);
        }

        CGDataProviderRelease(provider);
        CGColorSpaceRelease(space);

        CFRelease(file);
    }

    if (! success)
        std::cerr << "Failed to create " << outputFileNameAsCString << "!" << std::endl;
}

class CameraCodecCallback : public IBlackmagicRawCallback {
public:
    explicit CameraCodecCallback() = default;
    ~CameraCodecCallback() override = default;

    void ReadComplete(IBlackmagicRawJob* job, HRESULT result, IBlackmagicRawFrame* frame) override {
        IBlackmagicRawJob* procJob = nullptr;

        if (result == S_OK)
            VERIFY(frame->SetResourceFormat(s_resourceFormat));

        if (result == S_OK)
            result = frame->CreateJobDecodeAndProcessFrame(nullptr, nullptr, &procJob);

        if (result == S_OK)
            result = procJob->Submit();

        if (result != S_OK)
        {
            if (procJob)
                procJob->Release();
        }

        job->Release();
    }

    void ProcessComplete(IBlackmagicRawJob* job, HRESULT result, IBlackmagicRawProcessedImage* processedImage) override {
        unsigned int width = 0;
        unsigned int height = 0;
        unsigned int sizeBytes = 0;
        void* imageData = nullptr;

        if (result == S_OK)
            result = processedImage->GetWidth(&width);

        if (result == S_OK)
            result = processedImage->GetHeight(&height);

        if (result == S_OK)
            result = processedImage->GetResourceSizeBytes(&sizeBytes);

        if (result == S_OK)
            result = processedImage->GetResource(&imageData);

        if (result == S_OK)
            OutputImage(s_outputFileName, width, height, sizeBytes, imageData);

        job->Release();
    }
    void DecodeComplete(IBlackmagicRawJob*, HRESULT) override {}
    void TrimProgress(IBlackmagicRawJob*, float) override {}
    void TrimComplete(IBlackmagicRawJob*, HRESULT) override {}
    void SidecarMetadataParseWarning(IBlackmagicRawClip*, CFStringRef, uint32_t, CFStringRef) override {}
    void SidecarMetadataParseError(IBlackmagicRawClip*, CFStringRef, uint32_t, CFStringRef) override {}
    void PreparePipelineComplete(void*, HRESULT) override {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*) override
    {
        return E_NOTIMPL;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return 0;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        return 0;
    }
};

int main(int argc, char** argv) {
    std::string outDir;
    std::string inDir;
    std::string outputFormat;

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-f" || std::string(argv[i]) == "--outputFormat") {
            if (i + 1 < argc) {
                outputFormat = argv[++i];
            } else {
                std::cerr << "--outputFormat option requires one argument." << std::endl;
                return 1;
            }
        } else if (std::string(argv[i]) == "-i" || std::string(argv[i]) == "--inDir") {
            if (i + 1 < argc) {
                inDir = argv[++i];
            } else {
                std::cerr << "--inDir option requires one argument." << std::endl;
                return 1;
            }
        } else if (std::string(argv[i]) == "-o" || std::string(argv[i]) == "--outDir") {
            if (i + 1 < argc) {
                outDir = argv[++i];
            } else {
                std::cerr << "--outDir option requires one argument." << std::endl;
                return 1;
            }
        }
    }

    if (outputFormat.empty() || inDir.empty()) {
        std::cerr << "Usage: " << argv[0] << " -o/--outputFormat <outputFormat> -i/--inDir <inDir>" << std::endl;
        return 1;
    }

    if (outputFormat == "png") {
        s_resourceFormat = blackmagicRawResourceFormatRGBAU8;
    } else if (outputFormat == "dng") {
        s_resourceFormat = blackmagicRawResourceFormatRGBF16;
    } else {
        std::cerr << "Unknown output format: " << outputFormat << std::endl;
        return 1;
    }

    struct stat inInfo;
    if (stat(inDir.c_str(), &inInfo) != 0) {
        std::cerr << "Failed to stat input directory: " << inDir << std::endl;
        return 1;
    }

    struct stat outInfo;
    if (stat(outDir.c_str(), &outInfo) != 0) {
        std::cerr << "Failed to stat output directory: " << outDir << std::endl;
        return 1;
    }

    std::cout << "outDir: " << outDir << std::endl;
    std::cout << "inDir: " << inDir << std::endl;
    std::cout << "outputFormat: " << outputFormat << std::endl;
    std::cout << "----------" << std::endl;

    DIR* dir = opendir(inDir.c_str());
    if (dir == nullptr) {
        std::cerr << "Failed to open input directory: " << inDir << std::endl;
        return 1;
    }

    struct dirent* file;
    while ((file = readdir(dir)) != nullptr) {
        if (file->d_type != DT_REG) {
            continue;
        }

        std::string fileName = file->d_name;
        std::string extension = fileName.substr(fileName.find_last_of(".") + 1);
        if (extension != "braw") {
            continue;
        }

        std::string inPath = inDir + "/" + fileName;
        std::string outPath = outDir + "/" + file->d_name + "." + outputFormat;

        std::cout << "processing file " << inPath << std::endl;

        CFStringRef clipName = CFStringCreateWithCString( nullptr, inPath.c_str(), kCFStringEncodingUTF8);
        if (clipName == nullptr) {
            std::cerr << "Failed to create CFString from input path: " << inPath << std::endl;
            return 1;
        }

        IBlackmagicRawFactory* factory = CreateBlackmagicRawFactoryInstanceFromPath(s_brawSdkPath);
        if (factory == nullptr) {
            std::cerr << "Failed to create IBlackmagicRawFactory!" << std::endl;
            return 1;
        }

        IBlackmagicRaw* codec = nullptr;
        HRESULT result = factory->CreateCodec(&codec);
        if (result != S_OK) {
            std::cerr << "Failed to create IBlackmagicRaw!" << std::endl;
            return 1;
        }

        IBlackmagicRawClip* clip = nullptr;
        result = codec->OpenClip(clipName, &clip);
        if (result != S_OK) {
            std::cerr << "Failed to open IBlackmagicRawClip!" << std::endl;
            return 1;
        }

        s_outputFileName = CFStringCreateWithCString( nullptr, outPath.c_str(), kCFStringEncodingUTF8);

        CameraCodecCallback callback;
        result = codec->SetCallback(&callback);
        if (result != S_OK) {
            std::cerr << "Failed to set IBlackmagicRawCallback!" << std::endl;
            return 1;
        }

        IBlackmagicRawJob* readJob = nullptr;
        long readTime = 0;
        result = clip->CreateJobReadFrame(readTime, &readJob);
        if (result != S_OK) {
            std::cerr << "Failed to create IBlackmagicRawJob!" << std::endl;
            return 1;
        }

        result = readJob->Submit();
        if (result != S_OK) {
            std::cerr << "Failed to submit IBlackmagicRawJob!" << std::endl;
            return 1;
        }

        codec->FlushJobs();

        if (clip != nullptr)
            clip->Release();

        if (codec != nullptr)
            codec->Release();

        if (factory != nullptr)
            factory->Release();

        CFRelease(clipName);
    }

    return 0;
}
