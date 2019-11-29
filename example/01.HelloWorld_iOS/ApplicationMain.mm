#import <UIKit/UIKit.h>

@interface Controller : UIViewController

@end

@implementation Controller

- (void)viewDidLoad {
    self.view.backgroundColor = [UIColor redColor];

    [super viewDidLoad];
}

@end

@interface ApplicationDelegate : UIResponder<UIApplicationDelegate>

@property (strong, nonatomic) UIWindow* window;

@end

@implementation ApplicationDelegate

- (BOOL)application:(UIApplication*)pApplication didFinishLaunchingWithOptions:(NSDictionary*)pLaunchOptions
{
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    self.window.rootViewController = [[Controller alloc] init];
    [self.window makeKeyAndVisible];

    return YES;
}

@end

int main(int pArgumentsCount, char* pArguments[])
{
    NSString* className = nil;
    @autoreleasepool
    {
        className = NSStringFromClass([ApplicationDelegate class]);
    }
    
    return UIApplicationMain(pArgumentsCount, pArguments, nil, className);
}
