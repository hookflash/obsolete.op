#import "hookflashViewController.h"
#import <UIKit/UIKit.h>

NSString * const kOrientationChanged = @"orientationChanged";
char homeDirectory[256];

int hookflash_test(const char* fileName, int testID, IPhoneRenderView* renderView);

@implementation hookflashViewController

- (void) SendLastFrameToMainView:(UIImage*) image:(id) sender
{
    [_imgView setFrame:CGRectMake(_imgView.frame.origin.x, _imgView.frame.origin.y, image.size.width, image.size.height)];
    [_imgView performSelectorOnMainThread:@selector(setImage:)
                                      withObject:image waitUntilDone:NO];

}

- (void) SendLastFrameToSubView:(UIImage*) image:(id) sender
{
//    [_imgView setImage:image];
    [_imgView performSelectorOnMainThread:@selector(setImage:)
                               withObject:image waitUntilDone:NO];
}

#pragma mark - Start Test(s)
-(IBAction)startTest
{
    const char* home = [NSHomeDirectory() UTF8String];
    
    strcpy(homeDirectory, home);
    
    NSString* str = [NSHomeDirectory() stringByAppendingString:@"/Documents/video_capture_hookflash.txt"];
    
    const char* buffer = [str UTF8String];
    
    hookflash_test(buffer, 0, _renderView);
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        _renderView = [[IPhoneRenderView alloc] init];
        _renderView.viewId = 0;
        _renderView.delegate = self;
        self.title = @"Video Test";
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged) name:kOrientationChanged object:nil];

}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
      [[NSNotificationCenter defaultCenter] removeObserver:self name:kOrientationChanged object:nil];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

@end
