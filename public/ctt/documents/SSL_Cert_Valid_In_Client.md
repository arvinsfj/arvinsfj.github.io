
# SSL中的客户端安全策略（iOS平台）

> *作者：Arvin 日期：2017年12月21日*

---------------------------------

>BEGIN

本篇是阅读AFN库对iOS的Security框架中证书操作等功能的理解。客户端对服务器证书的信任策略，可以是不作校验完全信任，检验证书中的公钥，完全校验证书。原理很简单就是将服务器返回的证书（包含公钥、DomainName、有效期等）信息跟本地的证书信息进行比较，在安全策略的基础上判断是否终止数据链接。客户端主要解决服务器证书是否受信任的问题。SSL主要保证传输过程中数据的安全性和一致性（不受中间人攻击）。（不解决权限问题，比如：某个api客户端是否有权限进行数据访问）

### 一、思路
---------------------------------

在https请求挑战代理中，进行数据链接可信度判断，将服务器返回的证书（包含公钥、DomainName、有效期等）信息跟本地的证书信息进行比较，在安全策略的基础上判断是否终止数据链接（链接是否可信）。策略上粗略分成：完全可信、公钥可信、证书可信，三种策略。

### 二、主要api解析
---------------------------------

证书实体句柄的定义
```
typedef struct CF_BRIDGED_TYPE(id) SECTYPE(SecCertificate) *SecCertificateRef;
```

密钥实体句柄定义
```
typedef struct CF_BRIDGED_TYPE(id) SECTYPE(SecKey) *SecKeyRef;
```

信任策略实体句柄定义
```
typedef struct CF_BRIDGED_TYPE(id) SECTYPE(SecPolicy) *SecPolicyRef;
```

信任实体句柄的定义
```
typedef struct CF_BRIDGED_TYPE(id) __SecTrust *SecTrustRef;
```

信任结果状态枚举定义
```
typedef CF_ENUM(uint32_t, SecTrustResultType) {
    kSecTrustResultInvalid  CF_ENUM_AVAILABLE(10_3, 2_0) = 0,
    kSecTrustResultProceed  CF_ENUM_AVAILABLE(10_3, 2_0) = 1,
    kSecTrustResultConfirm  CF_ENUM_DEPRECATED(10_3, 10_9, 2_0, 7_0) = 2,
    kSecTrustResultDeny  CF_ENUM_AVAILABLE(10_3, 2_0) = 3,
    kSecTrustResultUnspecified  CF_ENUM_AVAILABLE(10_3, 2_0) = 4,
    kSecTrustResultRecoverableTrustFailure  CF_ENUM_AVAILABLE(10_3, 2_0) = 5,
    kSecTrustResultFatalTrustFailure  CF_ENUM_AVAILABLE(10_3, 2_0) = 6,
    kSecTrustResultOtherError  CF_ENUM_AVAILABLE(10_3, 2_0) = 7
};
```
两个带跳转断言宏的定义
```
#ifndef __Require_noErr_Quiet
	#define __Require_noErr_Quiet(errorCode, exceptionLabel)                      \
	  do                                                                          \
	  {                                                                           \
		  if ( __builtin_expect(0 != (errorCode), 0) )                            \
		  {                                                                       \
			  goto exceptionLabel;                                                \
		  }                                                                       \
	  } while ( 0 )
#endif
```

```
#ifndef __Require_Quiet
	#define __Require_Quiet(assertion, exceptionLabel)                            \
	  do                                                                          \
	  {                                                                           \
		  if ( __builtin_expect(!(assertion), 0) )                                \
		  {                                                                       \
			  goto exceptionLabel;                                                \
		  }                                                                       \
	  } while ( 0 )
#endif
```

下面是各种实体（句柄）创建和操作函数的定义。

密钥导出成NSData的函数
```
OSStatus SecItemExport(
     CFTypeRef                              secItemOrArray,
     SecExternalFormat                      outputFormat,
     SecItemImportExportFlags               flags,                   /* kSecItemPemArmor, etc. */
     const SecItemImportExportKeyParameters * __nullable keyParams,  /* optional */
     CFDataRef * __nonnull CF_RETURNS_RETAINED exportedData)         /* external representation returned here */
          __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_NA);
```

通过证书文件数据创建证书实体（句柄）函数
```
__nullable
SecCertificateRef SecCertificateCreateWithData(CFAllocatorRef __nullable allocator, CFDataRef data)
    __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_2_0);
```

信任策略实体（x.509默认策略）创建函数
```
SecPolicyRef SecPolicyCreateBasicX509(void)
    __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_2_0);
```

通过hostname和服务器选项创建信任策略（校验domain）
```
SecPolicyRef SecPolicyCreateSSL(Boolean server, CFStringRef __nullable hostname)
    __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_2_0);
```

通过证书实体和策略实体创建信任实体
```
OSStatus SecTrustCreateWithCertificates(CFTypeRef certificates,
    CFTypeRef __nullable policies, SecTrustRef * __nonnull CF_RETURNS_RETAINED trust)
    __OSX_AVAILABLE_STARTING(__MAC_10_3, __IPHONE_2_0);
```

对信任实体进行实际的校验，输出校验结果状态
```
OSStatus SecTrustEvaluate(SecTrustRef trust, SecTrustResultType * __nullable result)
    __OSX_AVAILABLE_STARTING(__MAC_10_3, __IPHONE_2_0);
```

信任实体中拷贝出公钥
```
__nullable
SecKeyRef SecTrustCopyPublicKey(SecTrustRef trust)
    __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_2_0);
```

获取信任实体中的证书数量
```
CFIndex SecTrustGetCertificateCount(SecTrustRef trust)
    __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_2_0);
```

获取信任实体中的证书
```
__nullable
SecCertificateRef SecTrustGetCertificateAtIndex(SecTrustRef trust, CFIndex ix)
    __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_2_0);
```

获取证书的Data（对应于证书文件的NSData）
```
CFDataRef SecCertificateCopyData(SecCertificateRef certificate)
    __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_2_0);
```

给信任实体设置信任策略实体
```
OSStatus SecTrustSetPolicies(SecTrustRef trust, CFTypeRef policies)
    __OSX_AVAILABLE_STARTING(__MAC_10_3, __IPHONE_6_0);
```

给信任实体设置锚点证书实体
```
OSStatus SecTrustSetAnchorCertificates(SecTrustRef trust,
    CFArrayRef anchorCertificates)
    __OSX_AVAILABLE_STARTING(__MAC_10_3, __IPHONE_2_0);
```

### 三、具体实现（服务器证书信息跟本地证书信息比较）
---------------------------------

本地证书哪里来的？实际是从服务器下载的保存在本地。SSL请求的时候，服务器会以证书的形式向客户端返回服务器证书，这个证书如果跟本地保存的证书一致就可以认为服务器是受信任的。

这个服务最核心的函数是：
```
- (BOOL)evaluateServerTrust:(SecTrustRef)serverTrust forDomain:(nullable NSString *)domain
{
    if (domain && self.allowInvalidCertificates && self.validatesDomainName && (self.SSLPinningMode == NKSSLPinningModeNone || [self.pinnedCertificates count] == 0)) {
        NSLog(@"In order to validate a domain name for self signed certificates, you MUST use pinning.");
        return NO;
    }
    
    NSMutableArray *policies = [NSMutableArray array];
    if (self.validatesDomainName) {
        [policies addObject:(__bridge_transfer id)SecPolicyCreateSSL(true, (__bridge CFStringRef)domain)];
    } else {
        [policies addObject:(__bridge_transfer id)SecPolicyCreateBasicX509()];
    }
    
    SecTrustSetPolicies(serverTrust, (__bridge CFArrayRef)policies);
    
    if (self.SSLPinningMode == NKSSLPinningModeNone) {
        return self.allowInvalidCertificates || NKServerTrustIsValid(serverTrust);
    } else if (!NKServerTrustIsValid(serverTrust) && !self.allowInvalidCertificates) {
        return NO;
    }
    
    switch (self.SSLPinningMode) {
        case NKSSLPinningModeNone:
        default:
            return NO;
        case NKSSLPinningModeCertificate: {
            NSMutableArray *pinnedCertificates = [NSMutableArray array];
            for (NSData *certificateData in self.pinnedCertificates) {
                [pinnedCertificates addObject:(__bridge_transfer id)SecCertificateCreateWithData(NULL, (__bridge CFDataRef)certificateData)];
            }
            SecTrustSetAnchorCertificates(serverTrust, (__bridge CFArrayRef)pinnedCertificates);
            if (!NKServerTrustIsValid(serverTrust)) {
                return NO;
            }
            NSArray *serverCertificates = NKCertificateTrustChainForServerTrust(serverTrust);
            for (NSData *trustChainCertificate in [serverCertificates reverseObjectEnumerator]) {
                if ([self.pinnedCertificates containsObject:trustChainCertificate]) {
                    return YES;
                }
            }
            return NO;
        }
        case NKSSLPinningModePublicKey: {
            NSUInteger trustedPublicKeyCount = 0;
            NSArray *publicKeys = NKPublicKeyTrustChainForServerTrust(serverTrust);
            
            for (id trustChainPublicKey in publicKeys) {
                for (id pinnedPublicKey in self.pinnedPublicKeys) {
                    if (NKSecKeyIsEqualToKey((__bridge SecKeyRef)trustChainPublicKey, (__bridge SecKeyRef)pinnedPublicKey)) {
                        trustedPublicKeyCount += 1;
                    }
                }
            }
            return trustedPublicKeyCount > 0;
        }
    }
    return NO;
}
```
上面的函数根据具体设置的安全策略，分别对服务器的证书进行校验，如果本地包含服务器的证书（公钥）等，则认为校验通过，服务器是受信任的。具体的本地和服务器证书实体、公钥如果得到，就需要前面讲的那些api了。

具体的使用
```
- (void)URLSession:(NSURLSession *)session
didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
 completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential *credential))completionHandler
{
    NSURLSessionAuthChallengeDisposition disposition = NSURLSessionAuthChallengePerformDefaultHandling;
    __block NSURLCredential *credential = nil;
    
    if ([challenge.protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust]) {
    	//服务器信任度校验
        if ([self.securityPolicy evaluateServerTrust:challenge.protectionSpace.serverTrust forDomain:challenge.protectionSpace.host]) {
            
            credential = [NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust];
            if (credential) {
                disposition = NSURLSessionAuthChallengeUseCredential;
            } else {
                disposition = NSURLSessionAuthChallengePerformDefaultHandling;
            }
        } else {
            disposition = NSURLSessionAuthChallengeCancelAuthenticationChallenge;
        }
    } else {
        disposition = NSURLSessionAuthChallengePerformDefaultHandling;
    }
    
    if (completionHandler) {
        completionHandler(disposition, credential);
    }
}
```
在NSURLSession的挑战代理中进行服务器证书信任度校验。如果校验通过则进行SSL后面的操作，如果没有通过校验则SSL链接被取消终止。

### 四、随便说点
---------------------------------

1. AFN只验证服务器证书是否可信
2. SSL可以保证数据传输过程的安全性和一致性
3. Security框架还有其他方面的api，比如：安全的套接字的实现、钥匙串的操作、手动创建证书、密钥加解密等
4. 还可以了解一下Apple体系对SSL方面是如何理解和架构的
5. Apple的企业证书可以打包出非appstore下的安装包，可以使用https服务器进行安装包的分发，如何实现一个简单的ipa分发服务呢？

### 五、附加HTTPS的建立过程
---------------------------------

HTTPS在传输数据之前需要客户端（浏览器）与服务端（网站）之间进行一次握手，在握手过程中将确立双方加密传输数据的密码信息。TLS/SSL协议不仅仅是一套加密传输的协议，更是一件经过艺术家精心设计的艺术品，TLS/SSL中使用了非对称加密，对称加密以及HASH算法。

握手过程的简单描述如下：

```
1. 浏览器将自己支持的一套加密规则发送给网站。
2. 网站从中选出一组加密算法与HASH算法，并将自己的身份信息以证书的形式发回给浏览器。证书里面包含了网站地址，加密公钥，以及证书的颁发机构等信息。
3. 获得网站证书之后浏览器要做以下工作：
a) 验证证书的合法性（颁发证书的机构是否合法，证书中包含的网站地址是否与正在访问的地址一致等），如果证书受信任，则浏览器栏里面会显示一个小锁头，否则会给出证书不受信的提示。
b) 如果证书受信任，或者是用户接受了不受信的证书，浏览器会生成一串随机数的密码，并用证书中提供的公钥加密。
c) 使用约定好的HASH计算握手消息，并使用生成的随机数对消息进行加密，最后将之前生成的所有信息发送给网站。
4. 网站接收浏览器发来的数据之后要做以下的操作：
a) 使用自己的私钥将信息解密取出密码，使用密码解密浏览器发来的握手消息，并验证HASH是否与浏览器发来的一致。
b) 使用密码加密一段握手消息，发送给浏览器。
5. 浏览器解密并计算握手消息的HASH，如果与服务端发来的HASH一致，此时握手过程结束，之后所有的通信数据将由之前浏览器生成的随机密码并利用对称加密算法进行加密。
```

这里浏览器与网站互相发送加密的握手消息并验证，目的是为了保证双方都获得了一致的密码，并且可以正常的加密解密数据，为后续真正数据的传输做一次测试。

另外，HTTPS一般使用的加密与HASH算法如下：
```
非对称加密算法：RSA，DSA/DSS
对称加密算法：AES，RC4，3DES
HASH算法：MD5，SHA1，SHA256
```

其中非对称加密算法用于在握手过程中加密生成的密码，对称加密算法用于对真正传输的数据进行加密，而HASH算法用于验证数据的完整性。由于浏览器生成的密码是整个数据加密的关键，因此在传输的时候使用了非对称加密算法对其加密。非对称加密算法会生成公钥和私钥，公钥只能用于加密数据，因此可以随意传输，而网站的私钥用于对数据进行解密，所以网站都会非常小心的保管自己的私钥，防止泄漏。
TLS握手过程中如果有任何错误，都会使加密连接断开，从而阻止了隐私信息的传输。

自己仔细分析上面的SSL过程，就可以理解为什么SSL传输数据的过程是安全的，为什么DDOS还是存在的。

iOS客户端（AFN）只做了"验证证书的合法性"(3.a)的工作，其他的工作都是iOS系统底层完成的。


>END

[代码下载](documents/TestHTTPS.zip)

