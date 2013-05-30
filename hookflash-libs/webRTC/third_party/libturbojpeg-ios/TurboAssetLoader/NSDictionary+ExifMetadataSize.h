//
//  NSDictionary+ExifMetadataSize.h
//  mps
//
//  Created by Johannes Schriewer on 16.01.2013.
//  Copyright (c) 2013 planetmutlu. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSDictionary (ExifMetadataSize)
- (CGSize)extractEXIFMetadataSize;

@end
