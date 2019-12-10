#import <UIKit/UIKit.h>

#include "HelloWorld.hpp"

@interface Controller : UIViewController
{
    __weak UILabel* mLabel;
    __weak UIButton* mButton;
    CGFloat mOffsetSize;
    std::unique_ptr<HelloWorld> mHelloWorld;
    int mRequestIndex;
}

@end

@implementation Controller

- (instancetype)init
{
    self = [super init];
    if (self != nil)
        [self setupWithOffset:0.0];
    
    return self;
}

- (instancetype)initWithOffset:(CGFloat)pOffset
{
    self = [super init];
    if (self != nil)
        [self setupWithOffset:pOffset];
    
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self buildLayout];
    
    [mButton addTarget:self action:@selector(execute) forControlEvents:UIControlEventTouchUpInside];
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
    UIStatusBarStyle statusBarStyle = UIStatusBarStyleDefault;
    if (@available(iOS 13.0, *))
        statusBarStyle = UIStatusBarStyleDarkContent;

    return statusBarStyle;
}

- (void)setupWithOffset:(CGFloat)pOffset
{
    mLabel = nil;
    mButton = nil;
    mOffsetSize = pOffset;
    mHelloWorld = std::make_unique<HelloWorld>(nullptr, "");
    mRequestIndex = 1;
}

- (void)execute
{
    mButton.enabled = NO;
    mLabel.text = @"";

    __weak UILabel* label = mLabel;
    __weak UIButton* button = mButton;
    mHelloWorld->execute([label, button](std::string lpOutputText) -> void
    {
        button.enabled = YES;
        label.text = [NSString stringWithUTF8String:lpOutputText.c_str()];
    }, mRequestIndex);
    
    mRequestIndex++;
    if (mRequestIndex > 7)
        mRequestIndex = 1;
}

- (void)buildLayout
{
    const CGFloat blockSize = self.view.bounds.size.width * 0.1;
    UIColor* const orange = [UIColor colorWithRed:1.0 green:0.51 blue:0.11 alpha:1.0];
    
    self.view.backgroundColor = [UIColor whiteColor];
    
    UILabel* label = [[UILabel alloc] initWithFrame:CGRectMake(blockSize, mOffsetSize + blockSize, self.view.bounds.size.width - blockSize * 2.0, blockSize * 3.0)];
    label.backgroundColor = orange;
    label.numberOfLines = 0;
    label.textAlignment = NSTextAlignmentLeft;
    label.font = [UIFont systemFontOfSize:blockSize * 0.6];
    label.textColor = [UIColor whiteColor];
    label.text = @"";
    [self.view addSubview:label];
    mLabel = label;
    
    UIButton* button = [[UIButton alloc] initWithFrame:CGRectMake(blockSize, mOffsetSize + blockSize * 5.0, self.view.bounds.size.width - blockSize * 2.0, blockSize * 1.5)];
    button.backgroundColor = orange;
    button.titleLabel.textAlignment = NSTextAlignmentCenter;
    button.titleLabel.font = [UIFont systemFontOfSize:blockSize];
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [button setTitle:@"EXECUTE" forState:UIControlStateNormal];
    [self.view addSubview:button];
    mButton = button;
}

@end

@interface ApplicationDelegate : UIResponder<UIApplicationDelegate>

@property (strong, nonatomic) UIWindow* window;

@end

@implementation ApplicationDelegate

- (BOOL)application:(UIApplication*)pApplication didFinishLaunchingWithOptions:(NSDictionary*)pLaunchOptions
{
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    
    CGFloat offsetSize = 0.0;
    if (@available(iOS 11.0, *))
        offsetSize = self.window.safeAreaInsets.top;
    
    self.window.rootViewController = [[Controller alloc] initWithOffset:offsetSize];
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
