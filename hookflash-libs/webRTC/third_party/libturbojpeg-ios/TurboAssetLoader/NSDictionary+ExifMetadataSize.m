//
//  NSDictionary+ExifMetadataSize.m
//  mps
//
//  Created by Johannes Schriewer on 16.01.2013.
//  Copyright (c) 2013 planetmutlu. All rights reserved.
//

#import "NSDictionary+ExifMetadataSize.h"

@implementation NSDictionary (ExifMetadataSize)

- (CGSize)extractEXIFMetadataSize {
    switch ([self[@"Orientation"] unsignedIntegerValue]) {
        case 1: // straight
        case 2: // mirrored
        case 3: // top down mirrored
        case 4: // top down
            return CGSizeMake([self[@"PixelWidth"] floatValue], [self[@"PixelHeight"] floatValue]);
        case 5: // rotated left side
        case 6: // rotated right side
        case 7: // rotated right side mirrored
        case 8: // rotated left side mirrored
            return CGSizeMake([self[@"PixelHeight"] floatValue], [self[@"PixelWidth"] floatValue]);
        default:
            NSLog(@"Unknown EXIF rotation");
            return CGSizeMake([self[@"PixelWidth"] floatValue], [self[@"PixelHeight"] floatValue]);
            break;
    }
}

@end
