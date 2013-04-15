//
//  ALAsset+ALAsset_turboloader.h
//  TurboAssetLoader
//
//  Created by Johannes Schriewer on 19.12.2012.
//  Copyright (c) 2012 Johannes Schriewer. All rights reserved.
//

#import <AssetsLibrary/AssetsLibrary.h>

@interface ALAssetRepresentation (DSTTurboLoader)

- (NSArray *)DSTQuickLoadSizes;
- (CGImageRef)DSTLoadImageOfSize:(CGSize)size;
- (CGSize)DSTQuickLoadSizeNear:(CGSize)desiredSize;
- (CGSize)DSTDimensions;

@end
