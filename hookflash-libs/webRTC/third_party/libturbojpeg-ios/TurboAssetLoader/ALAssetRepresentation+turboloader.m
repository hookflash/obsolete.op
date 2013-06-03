//
//  ALAsset+ALAsset_turboloader.m
//  TurboAssetLoader
//
//  Created by Johannes Schriewer on 19.12.2012.
//  Copyright (c) 2012 Johannes Schriewer. All rights reserved.
//

#import "ALAssetRepresentation+turboloader.h"

#import "turbojpeg.h"

#define ACCEPTED_ASPECT_ERROR 0.05

static void MemoryPlaneReleaseDataCallback (void *info, const void *data, size_t size) {
    free((void *)data);
}

@implementation ALAssetRepresentation (DSTTurboLoader)

- (NSArray *)DSTQuickLoadSizes {
    CGSize dimensions = [self DSTDimensions];

    if ([self.UTI isEqualToString:@"public.jpeg"]) {
        // ask libjpeg-turbo which sizes it supports natively, others will use Core Graphics to scale down
        int numFactors = 0;
        tjscalingfactor *factors = tjGetScalingFactors(&numFactors);

        NSMutableArray *result = [NSMutableArray arrayWithCapacity:numFactors];
        for (NSUInteger i = 0; i < numFactors; i++) {
            CGFloat zoom = (CGFloat)factors[i].num / (CGFloat)factors[i].denom;
            CGSize size = CGSizeMake(
                                     ceilf(dimensions.width * zoom),
                                     ceilf(dimensions.height * zoom)
                                     );
            [result addObject:[NSValue valueWithCGSize:size]];
        }
        return [NSArray arrayWithArray:result];
    }

    if ([self.UTI isEqualToString:@"public.png"]) {
        // define which sizes we can scale down fast
        NSArray *scale = @[ @(1.0), @(1.0/2.0), @(1.0/3.0), @(1.0/4.0), @(1.0/5.0), @(1.0/6.0), @(1.0/7.0), @(1.0/8.0) ];
        NSMutableArray *result = [NSMutableArray arrayWithCapacity:[scale count]];
        for (NSNumber *zoomFactor in scale) {
            CGFloat zoom = [zoomFactor floatValue];
            CGSize size = CGSizeMake(
                                     ceilf(dimensions.width * zoom),
                                     ceilf(dimensions.height * zoom)
                                     );
            [result addObject:[NSValue valueWithCGSize:size]];
        }
        return [NSArray arrayWithArray:result];
    }

    // Default to full size only
    return @[ [NSValue valueWithCGSize:dimensions] ];
}

- (CGSize)DSTQuickLoadSizeNear:(CGSize)desiredSize {
    CGSize dimensions = [self DSTDimensions];
    int numFactors = 0;
    tjscalingfactor *factors = tjGetScalingFactors(&numFactors);

    // Find the nearest possible scaling factor that turbojpeg supports
    CGSize delta = CGSizeMake(1000000, 1000000);
    CGSize nearest = CGSizeZero;
    for (NSUInteger i = 0; i < numFactors; i++) {
        CGFloat zoom = (CGFloat)factors[i].num / (CGFloat)factors[i].denom;
        CGSize size = CGSizeMake(
                                 ceilf(dimensions.width * zoom),
                                 ceilf(dimensions.height * zoom)
                                 );
        if ((size.width < desiredSize.width) || (size.height < desiredSize.height)) {
            continue;
        }
        if ((size.width == desiredSize.width) && (size.height == desiredSize.height)) {
            nearest = size;
            break;
        }
        if (size.width - desiredSize.width < delta.width) {
            delta.width = size.width - desiredSize.width;
            nearest = size;
        }
        if (size.height - desiredSize.height < delta.height) {
            delta.height = size.height - desiredSize.height;
            nearest = size;
        }
    }
    if (nearest.width == 0.0) {
        nearest = dimensions;
    }
    return nearest;
}

- (CGImageRef)DSTLoadImageOfSize:(CGSize)size {
    size = CGSizeMake(ceilf(size.width), ceilf(size.height));
    CGSize dimensions = [self DSTDimensions];

    // do not stretch images, only zoom
    if (dimensions.width > dimensions.height) {
        CGFloat imageAspect = dimensions.height / dimensions.width;
        CGFloat sizeAspect = size.height / size.width;
        if (fabsf(imageAspect - sizeAspect) > ACCEPTED_ASPECT_ERROR) {
            // image would be stretched, so we recalculate correct height
            size.height = ceilf(imageAspect * size.width);
        }
    } else {
        CGFloat imageAspect = dimensions.width / dimensions.height;
        CGFloat sizeAspect = size.width / size.height;
        if (fabsf(imageAspect - sizeAspect) > ACCEPTED_ASPECT_ERROR) {
            // image would be stretched, so we recalculate correct height
            size.width = ceilf(imageAspect * size.height);
        }
    }
    

    if ([self.UTI isEqualToString:@"public.jpeg"]) {
        // load image with turbojpeg library near the correct size and then downscale
        return [self DSTTurboLoadJPEGImage:size];
    }
    if ([self.UTI isEqualToString:@"public.png"]) {
        // TODO: load PNG image with libpng
    }

    return [self DSTDownscaleImage:[self fullResolutionImage] size:size correctExifRotation:YES];
}

#pragma mark - Private
- (CGSize)DSTDimensions {
    if ([self respondsToSelector:@selector(dimensions)]) {
        CGSize dimensions = self.dimensions;
        // looks like an iOS Bug to me:
        if (self.orientation == ALAssetOrientationLeftMirrored) {
            CGFloat tmp = dimensions.width;
            dimensions.width = dimensions.height;
            dimensions.height = tmp;
        }
        return dimensions;
    } else { // iOS 5.0 fallback
        CGSize dimensions;

        // Exif rotation
        switch (self.orientation) {
            case ALAssetOrientationUp:
            case ALAssetOrientationUpMirrored:
            case ALAssetOrientationDown:
            case ALAssetOrientationDownMirrored:
                dimensions = CGSizeMake([self.metadata[@"PixelWidth"] floatValue], [self.metadata[@"PixelHeight"] floatValue]);
                break;
            case ALAssetOrientationLeft:
            case ALAssetOrientationLeftMirrored:
            case ALAssetOrientationRight:
            case ALAssetOrientationRightMirrored:
                dimensions = CGSizeMake([self.metadata[@"PixelHeight"] floatValue], [self.metadata[@"PixelWidth"] floatValue]);
                break;
        }
        return dimensions;
    }
}

- (CGImageRef)DSTTurboLoadJPEGImage:(CGSize)desiredSize {
    NSAssert([self.UTI isEqualToString:@"public.jpeg"], @"Only call for JPEG");
    CGSize nearest = [self DSTQuickLoadSizeNear:desiredSize];

    // Exif rotation
    switch (self.orientation) {
        case ALAssetOrientationUp:
        case ALAssetOrientationUpMirrored:
        case ALAssetOrientationDown:
        case ALAssetOrientationDownMirrored:
            // do nothing
            break;
        case ALAssetOrientationLeft:
        case ALAssetOrientationLeftMirrored:
        case ALAssetOrientationRight:
        case ALAssetOrientationRightMirrored: {
            // 90 degrees rotated
            CGFloat tmp = nearest.width;
            nearest.width = nearest.height;
            nearest.height = tmp;
            break;
        }
    }

    // decompress image (may be bigger than desired size)
    tjhandle handle = tjInitDecompress();
    NSError *error = nil;
    uint8_t *buffer = malloc(self.size);
    [self getBytes:buffer fromOffset:0 length:self.size error:&error];
    if (error) {
        NSLog(@"Error fetching image data: %@", error);
        return nil;
    }

    uint8_t *imageBuffer = malloc(nearest.width * 4 * nearest.height);
    int success = tjDecompress2(handle, buffer, self.size, imageBuffer, nearest.width, nearest.width * 4, nearest.height, TJPF_BGRA, TJFLAG_FASTUPSAMPLE | TJFLAG_FASTDCT);
    free(buffer);
    if (success < 0) {
        NSLog(@"Error while decoding image data: %s", tjGetErrorStr());
        free(imageBuffer);
        tjDestroy(handle);
        return nil;
    }
    tjDestroy(handle);

    // Create CGImage from Buffer directly to avoid copy operation to context
    CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef rawImageDataProvider = CGDataProviderCreateWithData(nil, imageBuffer, nearest.width * 4 * nearest.height, MemoryPlaneReleaseDataCallback);
    CGImageRef image = CGImageCreate(nearest.width, nearest.height, 8, 8 * 4, nearest.width * 4, colorspace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little, rawImageDataProvider, NULL, YES, kCGRenderingIntentDefault);
    CGDataProviderRelease(rawImageDataProvider);
    CGColorSpaceRelease(colorspace);

    if ((self.orientation == ALAssetOrientationUp) && (nearest.width == desiredSize.width) && (nearest.height == desiredSize.height)) {
        return image; // no rotation or scaling neccessary
    }

    // scaling and orientation correction
    CGImageRef result = [self DSTDownscaleImage:image size:desiredSize correctExifRotation:YES];
    CGImageRelease(image);
    return result;
}

- (CGImageRef)DSTDownscaleImage:(CGImageRef)image size:(CGSize)size correctExifRotation:(BOOL)correctRotation {
    size = CGSizeMake(ceilf(size.width), ceilf(size.height));
    CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(NULL, size.width, size.height, 8, size.width * 4, colorspace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);

    CGContextSetInterpolationQuality(context, kCGInterpolationLow);

    CGContextTranslateCTM(context, size.width / 2.0, size.height / 2.0);
    CGRect imageRect = CGRectMake(-size.width / 2.0, -size.height / 2.0, size.width, size.height);

    if (correctRotation) {
        imageRect = [self DSTExifCorrectContext:context contextSize:size];
    }

    CGContextDrawImage(context, imageRect, image);
    CGImageRef result = CGBitmapContextCreateImage(context);
    CGContextRelease(context);
    CGColorSpaceRelease(colorspace);
    return result;
}

- (CGRect)DSTExifCorrectContext:(CGContextRef)context contextSize:(CGSize)size {
    CGRect imageRect = CGRectMake(-size.width / 2.0, -size.height / 2.0, size.width, size.height);

    // Exif rotation
    switch (self.orientation) {
        case ALAssetOrientationUp:
        case ALAssetOrientationUpMirrored:
            // do nothing
            break;
        case ALAssetOrientationDown:
        case ALAssetOrientationDownMirrored:
            // asset on top rotate 180 degrees
            CGContextRotateCTM(context, M_PI);
            break;
        case ALAssetOrientationLeft:
        case ALAssetOrientationLeftMirrored:
            // asset on left, rotate 90 degrees
            imageRect = CGRectMake(-size.height / 2.0, -size.width/ 2.0, size.height, size.width);
            CGContextRotateCTM(context, M_PI_2);
            break;
        case ALAssetOrientationRight:
        case ALAssetOrientationRightMirrored:
            // asset on left, rotate -90 degrees
            imageRect = CGRectMake(-size.height / 2.0, -size.width/ 2.0, size.height, size.width);
            CGContextRotateCTM(context, -M_PI_2);
            break;
    }

    // Exif mirroring
    switch (self.orientation) {
        case ALAssetOrientationUp:
        case ALAssetOrientationDown:
        case ALAssetOrientationLeft:
        case ALAssetOrientationRight:
            // do nothing
            break;
        case ALAssetOrientationUpMirrored:
        case ALAssetOrientationLeftMirrored:
        case ALAssetOrientationRightMirrored:
        case ALAssetOrientationDownMirrored:
            // de-mirror
            CGContextScaleCTM(context, -1, 1);
            break;
    }

    return imageRect;
}

@end
