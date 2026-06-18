#pragma once

#include <string>
#include <vector>

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>

namespace sim {

struct LoadedImage {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgba;
};

inline LoadedImage loadImageRgba8(const std::string& path) {
    LoadedImage result;
    @autoreleasepool {
        NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
        NSURL* url = [NSURL fileURLWithPath:nsPath];
        CGImageSourceRef source = CGImageSourceCreateWithURL((__bridge CFURLRef)url, nullptr);
        if (source == nullptr) {
            return result;
        }

        CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
        CFRelease(source);
        if (image == nullptr) {
            return result;
        }

        result.width = static_cast<int>(CGImageGetWidth(image));
        result.height = static_cast<int>(CGImageGetHeight(image));
        if (result.width <= 0 || result.height <= 0) {
            CGImageRelease(image);
            result = {};
            return result;
        }

        result.rgba.resize(static_cast<std::size_t>(result.width) *
                           static_cast<std::size_t>(result.height) * 4U);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        const CGBitmapInfo bitmapInfo =
            static_cast<CGBitmapInfo>(kCGImageAlphaPremultipliedLast) |
            kCGBitmapByteOrder32Big;
        CGContextRef context = CGBitmapContextCreate(
            result.rgba.data(),
            static_cast<std::size_t>(result.width),
            static_cast<std::size_t>(result.height),
            8,
            static_cast<std::size_t>(result.width) * 4U,
            colorSpace,
            bitmapInfo);
        if (context != nullptr) {
            CGContextDrawImage(
                context,
                CGRectMake(0.0, 0.0, static_cast<CGFloat>(result.width), static_cast<CGFloat>(result.height)),
                image);
            CGContextRelease(context);
        } else {
            result = {};
        }
        CGColorSpaceRelease(colorSpace);
        CGImageRelease(image);
    }
    return result;
}

}  // namespace sim
