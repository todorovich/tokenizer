#ifndef LIBFPE_JNI_HPP
#define LIBFPE_JNI_HPP

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

    JNIEXPORT jlong JNICALL Java_net_todorovich_fpe_jdbc_UnicodeFPECipherJNI_create(JNIEnv* env, jobject obj);

    JNIEXPORT jint JNICALL Java_net_todorovich_fpe_jdbc_UnicodeFPECipherJNI_encrypt(
        JNIEnv* env, jobject obj, jlong handle,
        jbyteArray input, jint inputLen,
        jbyteArray output, jint outputCapacity);

    JNIEXPORT jint JNICALL Java_net_todorovich_fpe_jdbc_UnicodeFPECipherJNI_decrypt(
        JNIEnv* env, jobject obj, jlong handle,
        jbyteArray input, jint inputLen,
        jbyteArray output, jint outputCapacity);

    JNIEXPORT void JNICALL Java_net_todorovich_fpe_jdbc_UnicodeFPECipherJNI_destroy(JNIEnv* env, jobject obj, jlong handle);

#ifdef __cplusplus
}
#endif

#endif // LIBFPE_JNI_HPP
