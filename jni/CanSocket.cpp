#include<string>
#include<algorithm>
#include<utility>

#include<cstring>
#include<cstddef>
#include<cerrno>

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <net/if.h>
#include "include/linux/can.h"
#include "include/linux/can/raw.h"
#include "include/debug.h"

#if defined(ANDROID) || defined(__ANDROID__)
#include "jni.h"
#endif


#ifndef PF_CAN
#define PF_CAN 29
#endif

#ifndef AF_CAN
#define AF_CAN PF_CAN
#endif

static const int ERRNO_BUFFER_LEN = 1024;

/**
 * 抛出异常方法
 */
static void throwException(JNIEnv *env, const std::string& exception_name,
			   const std::string& msg)
{
	const jclass exception = env->FindClass(exception_name.c_str());
	if (exception == NULL) {
		return;
	}
	env->ThrowNew(exception, msg.c_str());
}

/**
 * 抛出IO异常
 */
static void throwIOExceptionMsg(JNIEnv *env, const std::string& msg)
{
	throwException(env, "java/io/IOException", msg);
}

/**
 * 抛出errno对应的异常信息
 */
static void throwIOExceptionErrno(JNIEnv *env, const int exc_errno)
{
	char message[ERRNO_BUFFER_LEN];
	// The strerror() function returns a pointer to a string that describes the error code
	const char *const msg = (char *) strerror_r(exc_errno, message, ERRNO_BUFFER_LEN);
	if (((long)msg) == 0) {
		// POSIX strerror_r, success
		throwIOExceptionMsg(env, std::string(message));
	} else if (((long)msg) == -1) {
		// POSIX strerror_r, failure
		// (Strictly, POSIX only guarantees a value other than 0. The safest
		// way to implement this function is to use C++ and overload on the
		// type of strerror_r to accurately distinguish GNU from POSIX. But
		// realistic implementations will always return -1.)
		snprintf(message, ERRNO_BUFFER_LEN, "errno %d", exc_errno);
		throwIOExceptionMsg(env, std::string(message));
	} else {
		// glibc strerror_r returning a string
		throwIOExceptionMsg(env, std::string(msg));
	}
}

/**
 * 抛出参数传递异常
 */
static void throwIllegalArgumentException(JNIEnv *env, const std::string& message)
{
	throwException(env, "java/lang/IllegalArgumentException", message);
}

/**
 * 内存越界异常
 */
static void throwOutOfMemoryError(JNIEnv *env, const std::string& message)
{
    	throwException(env, "java/lang/OutOfMemoryError", message);
}

/**
 * 生成一个socket CAN类型的文件描述符，并返回。
 * 如果创建成功，则直接返回文件描述符，如果创建失败则通过errno查询并抛出IO异常信息。
 */
static jint newCanSocket(JNIEnv *env, int socket_type, int protocol)
{
	const int fd = socket(PF_CAN, socket_type, protocol);
	if (fd != -1) {
		return fd;
	}
	throwIOExceptionErrno(env, errno);
	return -1;
}

/**
 * JNI直接调用的soketCan方法, the raw socket protocol
 */
JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1openSocketRAW
(JNIEnv *env, jclass obj)
{
	return newCanSocket(env, SOCK_RAW, CAN_RAW);
}


/**
 * JNI直接调用的soketCan方法，the broadcast manager
 */
JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1openSocketBCM
(JNIEnv *env, jclass obj)
{
	return newCanSocket(env, SOCK_DGRAM, CAN_BCM);
}

/**
 * JNI关闭socketCan方法，如果出现错误，则抛出异常
 */
JNIEXPORT void JNICALL Java_com_android_socketcan_CanSocket__1close
(JNIEnv *env, jclass obj, jint fd)
{
	if (close(fd) == -1) {
		throwIOExceptionErrno(env, errno);
	}
}

/**
 * 通过接口名（如can0、can1）查找CAN接口索引
 */
JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1discoverInterfaceIndex
(JNIEnv *env, jclass clazz, jint socketFd, jstring ifName)
{
	struct ifreq ifreq;
	// 检查接口名是否符合要求
	const jsize ifNameSize = env->GetStringUTFLength(ifName);
	if (ifNameSize > IFNAMSIZ-1) {
		throwIllegalArgumentException(env, "illegal interface name");
		return -1;
	}

	/* fetch interface name */
	// 将Java中UTF编码的字符串转成C里面的的字符串并设定到ifreq结构体中对应的域（ifr_name)
	memset(&ifreq, 0x0, sizeof(ifreq));
	env->GetStringUTFRegion(ifName, 0, ifNameSize,
				ifreq.ifr_name);
	if (env->ExceptionCheck() == JNI_TRUE) {
		return -1;
	}

	/* discover interface id */
	// 查找ifName在内核驱动中对应的CAN设备序号
	const int err = ioctl(socketFd, SIOCGIFINDEX, &ifreq);
	if (err == -1) {
		throwIOExceptionErrno(env, errno);
		return -1;
	} else {
		return ifreq.ifr_ifindex;
	}
}

/**
 * 通过内核中的CAN设备对应的序号（如0、1）查找接口的名字（如can0，can1）
 */
JNIEXPORT jstring JNICALL Java_com_android_socketcan_CanSocket__1discoverInterfaceName
(JNIEnv *env, jclass obj, jint fd, jint ifIdx)
{
	struct ifreq ifreq;
	// 初始化ifreq
	memset(&ifreq, 0x0, sizeof(ifreq));

	// 设置index（序号），并获取ifreq
	ifreq.ifr_ifindex = ifIdx;
	if (ioctl(fd, SIOCGIFNAME, &ifreq) == -1) {
		throwIOExceptionErrno(env, errno);
		return NULL;
	}

	// 将C字符串转成Java中的UTF编码的字符串
	const jstring ifname = env->NewStringUTF(ifreq.ifr_name);
	return ifname;
}

/**
 * 初始化sockaddr结构体，并bind到对应的fd上。
 */
JNIEXPORT void JNICALL Java_com_android_socketcan_CanSocket__1bindToSocket
(JNIEnv *env, jclass obj, jint fd, jint ifIndex)
{
	struct sockaddr_can addr;
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifIndex;
	if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) {
		throwIOExceptionErrno(env, errno);
	}
}

/**
 * 发送一帧数据
 */
JNIEXPORT void JNICALL Java_com_android_socketcan_CanSocket__1sendFrame
(JNIEnv *env, jclass obj, jint fd, jint if_idx, jint canid, jbyteArray data)
{
	const int flags = 0;
	ssize_t nbytes;
	struct sockaddr_can addr;
	struct can_frame frame;

	// 设置sockaddr_can
	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = if_idx;

	// 初始化can_frame
	memset(&frame, 0, sizeof(frame));

	// 获取数据长度，并检查否出错
	const jsize len = env->GetArrayLength(data);
	if (env->ExceptionCheck() == JNI_TRUE) {
		return;
	}

	/**
	 * // CAN payload length and DLC definitions according to ISO 11898-1
	 * #define CAN_MAX_DLC 8
	 * #define CAN_MAX_DLEN 8
	 *
	 * //
	 * // struct can_frame - basic CAN frame structure
	 * // @can_id:  CAN ID of the frame and CAN_*_FLAG flags, see canid_t definition
	 * // @can_dlc: frame payload length in byte (0 .. 8) aka data length code
	 * //           N.B. the DLC field from ISO 11898-1 Chapter 8.4.2.3 has a 1:1
	 * //           mapping of the 'data length code' to the real payload length
	 * // @__pad:   padding
	 * // @__res0:  reserved / padding
	 * // @__res1:  reserved / padding
	 * // @data:    CAN frame payload (up to 8 byte)
	 * //
	 * struct can_frame {
	 *         canid_t can_id;  // 32 bit CAN_ID + EFF/RTR/ERR flags
	 *         __u8    can_dlc; // frame payload length in byte (0 .. CAN_MAX_DLEN)
	 *         __u8    __pad;   // padding
	 *         __u8    __res0;  // reserved / padding
	 *         __u8    __res1;  // reserved / padding
	 *         __u8    data[CAN_MAX_DLEN] __attribute__((aligned(8)));
	 * };
	 *
	 * 这里要注意，一帧的数据长度不能长于8字节，因为如上面所示内核中定义的数据长度不能长于8字节
	 */
	frame.can_id = canid;
	frame.can_dlc = static_cast<__u8>(len);
	env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte *>(&frame.data));
	if (env->ExceptionCheck() == JNI_TRUE) {
		return;
	}

	// 发送设置好的一帧数据
	nbytes = sendto(fd, &frame, sizeof(frame), flags,
			reinterpret_cast<struct sockaddr *>(&addr),
			sizeof(addr));
	if (nbytes == -1) {
		throwIOExceptionErrno(env, errno);
	} else if (nbytes != sizeof(frame)) {
		throwIOExceptionMsg(env, "send partial frame");
	}
}

/**
 * 接收一帧数据
 */
JNIEXPORT jobject JNICALL Java_com_android_socketcan_CanSocket__1recvFrame
(JNIEnv *env, jclass obj, jint fd)
{
	const int flags = 0;
	ssize_t nbytes;
	struct sockaddr_can addr;
	socklen_t len = sizeof(addr);
	struct can_frame frame;

	// 初始化接收数据结构
	memset(&addr, 0, sizeof(addr));
	memset(&frame, 0, sizeof(frame));

	// 接收数据，并判断，如果数据有问题，抛出异常信息
	nbytes = recvfrom(fd, &frame, sizeof(frame), flags,
			  reinterpret_cast<struct sockaddr *>(&addr), &len);
	if (len != sizeof(addr)) {
		throwIllegalArgumentException(env, "illegal AF_CAN address");
		return NULL;
	}
	if (nbytes == -1) {
		throwIOExceptionErrno(env, errno);
		return NULL;
	} else if (nbytes != sizeof(frame)) {
		throwIOExceptionMsg(env, "invalid length of received frame");
		return NULL;
	}

	/**
	 * 收到的数据一定是小于等于8字节
	 * nbytes - offsetof(struct can_frame, data)) 相当于一共收到了nbytes字节减去data在struct can_frame结构体中的的偏移，
	 * 其结果应该和frame.can_dlc是一样的。
	 */
	const jsize fsize = static_cast<jsize>(
			std::min(
					static_cast<size_t>( frame.can_dlc),
					static_cast<size_t>( nbytes - offsetof(struct can_frame, data))
			)
	);
	const jclass can_frame_clazz = env->FindClass("com/android/socketcan/"
							"CanSocket$CanFrame");
	if (can_frame_clazz == NULL) {
		return NULL;
	}

	/**
	 * 1. 参考文档：
	 * 	  1. 对于JNI方法名，数据类型和方法签名
	 * 	      http://blog.csdn.net/leewokan/article/details/51338085
	 * 	  2. Android NDK之----- C调用Java [GetMethodID方法的使用]
	 * 	  	  http://blog.csdn.net/go_to_learn/article/details/7572372
	 * 2. 解析：
	 * 	  1. <init>: 调用构造函数；
	 * 	  2. (II[B)V: int canIf, int canid, byte[] data
	 * 	  3. 调用函数：
     *       private CanFrame(int canIf, int canid, byte[] data) {
     *           if (data.length > 8) {
     *               throw new IllegalArgumentException();
     *           }
     *           this.canIf = new CanInterface(canIf);
     *           this.canId = new CanId(canid);
     *           this.data = data;
     *       }
	 */
	const jmethodID can_frame_cstr = env->GetMethodID(can_frame_clazz,
							"<init>", "(II[B)V");
	if (can_frame_cstr == NULL) {
		return NULL;
	}

	// 创建Java类型的字节数组，并把接收到的一帧的内容赋值到Java数组中去
	const jbyteArray data = env->NewByteArray(fsize);
	if (data == NULL) {
		if (env->ExceptionCheck() != JNI_TRUE) {
			throwOutOfMemoryError(env, "could not allocate ByteArray");
		}
		return NULL;
	}
	env->SetByteArrayRegion(data, 0, fsize, reinterpret_cast<jbyte *>(&frame.data));
	if (env->ExceptionCheck() == JNI_TRUE) {
		return NULL;
	}

	// 创建一个CanFrame对象
	const jobject ret = env->NewObject(can_frame_clazz, can_frame_cstr,
					   addr.can_ifindex, frame.can_id,
					   data);
	return ret;
}

/**
 * Get or set the MTU (Maximum Transfer Unit) of a device using ifr_mtu. Setting the MTU is a privileged operation.
 * Setting the MTU to too small values may cause kernel crashes.
 */
JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1fetchInterfaceMtu
(JNIEnv *env, jclass obj, jint fd, jstring ifName)
{
	struct ifreq ifreq;

	const jsize ifNameSize = env->GetStringUTFLength(ifName);
	if (ifNameSize > IFNAMSIZ-1) {
		throwIllegalArgumentException(env, "illegal interface name");
		return -1;
	}

	memset(&ifreq, 0x0, sizeof(ifreq));
	env->GetStringUTFRegion(ifName, 0, ifNameSize, ifreq.ifr_name);
	if (env->ExceptionCheck() == JNI_TRUE) {
		return -1;
	}

	if (ioctl(fd, SIOCGIFMTU, &ifreq) == -1) {
		throwIOExceptionErrno(env, errno);
		return -1;
	} else {
		return ifreq.ifr_mtu;
	}
}

/**
 * 设置socket的option选项
 */
JNIEXPORT void JNICALL Java_com_android_socketcan_CanSocket__1setsockopt
(JNIEnv *env, jclass obj, jint fd, jint op, jint stat)
{
	const int _stat = stat;
	if (setsockopt(fd, SOL_CAN_RAW, op, &_stat, sizeof(_stat)) == -1) {
		throwIOExceptionErrno(env, errno);
	}
}

/**
 * 获取socket的option选项
 */
JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1getsockopt
(JNIEnv *env, jclass obj, jint fd, jint op)
{
	int _stat = 0;
	socklen_t len = sizeof(_stat);
	if (getsockopt(fd, SOL_CAN_RAW, op, &_stat, &len) == -1) {
		throwIOExceptionErrno(env, errno);
	}
	if (len != sizeof(_stat)) {
		throwIllegalArgumentException(env, "setsockopt return size is different");
		return -1;
	}
	return _stat;
}


/*** constants ***/

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1fetch_1CAN_1MTU
(JNIEnv *env, jclass obj)
{
	/**
	 * struct can_frame {
	 *         canid_t can_id;  // 32 bit CAN_ID + EFF/RTR/ERR flags
	 *         __u8    len;     // frame payload length in byte
	 *         __u8    flags;   // additional flags for CAN FD
	 *         __u8    __res0;  // reserved / padding
	 *         __u8    __res1;  // reserved / padding
	 *         __u8    data[CAN_MAX_DLEN] __attribute__((aligned(8)));
	 * };
	 *
	 * #define CAN_MTU         (sizeof(struct can_frame))
	 */
	return CAN_MTU;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1fetch_1CAN_1FD_1MTU
(JNIEnv *env, jclass obj)
{
	/**
	 * struct canfd_frame {
	 *         canid_t can_id;  // 32 bit CAN_ID + EFF/RTR/ERR flags
	 *         __u8    len;     // frame payload length in byte
	 *         __u8    flags;   // additional flags for CAN FD
	 *         __u8    __res0;  // reserved / padding
	 *         __u8    __res1;  // reserved / padding
	 *         __u8    data[CANFD_MAX_DLEN] __attribute__((aligned(8)));
	 * };
	 *
	 * #define CANFD_MTU       (sizeof(struct canfd_frame))
	 */
	return CANFD_MTU;
}

/*** ioctls ***/
JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1fetch_1CAN_1RAW_1FILTER
(JNIEnv *env, jclass obj)
{
	return CAN_RAW_FILTER;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1fetch_1CAN_1RAW_1ERR_1FILTER
(JNIEnv *env, jclass obj)
{
	return CAN_RAW_ERR_FILTER;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1fetch_1CAN_1RAW_1LOOPBACK
(JNIEnv *env, jclass obj)
{
	return CAN_RAW_LOOPBACK;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1fetch_1CAN_1RAW_1RECV_1OWN_1MSGS
(JNIEnv *env, jclass obj)
{
	return CAN_RAW_RECV_OWN_MSGS;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1fetch_1CAN_1RAW_1FD_1FRAMES
(JNIEnv *env, jclass obj)
{
	return CAN_RAW_FD_FRAMES;
}

/*** ADR MANIPULATION FUNCTIONS ***/

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1getCANID_1SFF
(JNIEnv *env, jclass obj, jint canid)
{
	// SFF: standard Frame Format
	return canid & CAN_SFF_MASK;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1getCANID_1EFF
(JNIEnv *env, jclass obj, jint canid)
{
	// SFF: extern Frame Format
	return canid & CAN_EFF_MASK;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1getCANID_1ERR
(JNIEnv *env, jclass obj, jint canid)
{
	// error
	return canid & CAN_ERR_MASK;
}

JNIEXPORT jboolean JNICALL Java_com_android_socketcan_CanSocket__1isSetEFFSFF
(JNIEnv *env, jclass obj, jint canid)
{
	// 判断是标准帧还是扩展帧
	return (canid & CAN_EFF_FLAG) != 0 ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_android_socketcan_CanSocket__1isSetRTR
(JNIEnv *env, jclass obj, jint canid)
{
	// 判断是否是远程帧，还是数据帧
	return (canid & CAN_RTR_FLAG) != 0 ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_android_socketcan_CanSocket__1isSetERR
(JNIEnv *env, jclass obj, jint canid)
{
	// 判断是否有error
	return (canid & CAN_ERR_FLAG) != 0 ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1setEFFSFF
(JNIEnv *env, jclass obj, jint canid)
{
	// 设置标准帧还是扩展帧
	return canid | CAN_EFF_FLAG;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1setRTR
(JNIEnv *env, jclass obj, jint canid)
{
	// 设置远程帧
	return canid | CAN_RTR_FLAG;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1setERR
(JNIEnv *env, jclass obj, jint canid)
{
	// 设置错误位
	return canid | CAN_ERR_FLAG;
}

/**
 * 设置为标准帧
 */
JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1clearEFFSFF
(JNIEnv *env, jclass obj, jint canid)
{
	// 清除扩展帧
	return canid & ~CAN_EFF_FLAG;
}

/**
 * 设置为数据帧
 */
JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1clearRTR
(JNIEnv *env, jclass obj, jint canid)
{
	// 清除远程帧
	return canid & ~CAN_RTR_FLAG;
}

JNIEXPORT jint JNICALL Java_com_android_socketcan_CanSocket__1clearERR
(JNIEnv *env, jclass obj, jint canid)
{
	// 清除错误位
	return canid & ~CAN_ERR_FLAG;
}

#ifdef __cplusplus
}
#endif
