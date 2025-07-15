#include "libfpe-jni.hpp"
#include <cstdint>
#include <cstring>
#include "libfpe.hpp"  // Your core FPE header

extern "C" {

    JNIEXPORT jlong JNICALL Java_net_todorovich_fpe_jdbc_UnicodeFPECipherJNI_create(JNIEnv* env, jobject obj) {
        return (jlong)unicodefpe_create();
    }

    JNIEXPORT jint JNICALL Java_net_todorovich_fpe_jdbc_UnicodeFPECipherJNI_encrypt(
        JNIEnv* env, jobject obj, jlong handle,
        jbyteArray input, jint inputLen,
        jbyteArray output, jint outputCapacity)
    {
        if (!input || !output) return 1;

        jbyte* inputBytes = env->GetByteArrayElements(input, NULL);
        jbyte* outputBytes = env->GetByteArrayElements(output, NULL);
        if (!inputBytes || !outputBytes) return 1;

        int ret = unicodefpe_encrypt(
            (UnicodeFPECipherHandle)handle,
            (const char*)inputBytes, (size_t)inputLen,
            (char*)outputBytes, (size_t)outputCapacity
        );

        env->ReleaseByteArrayElements(output, outputBytes, 0);
        env->ReleaseByteArrayElements(input, inputBytes, JNI_ABORT);

        return ret;
    }

    JNIEXPORT jint JNICALL Java_net_todorovich_fpe_jdbc_UnicodeFPECipherJNI_decrypt(
        JNIEnv* env, jobject obj, jlong handle,
        jbyteArray input, jint inputLen,
        jbyteArray output, jint outputCapacity)
    {
        if (!input || !output) return 1;

        jbyte* inputBytes = env->GetByteArrayElements(input, NULL);
        jbyte* outputBytes = env->GetByteArrayElements(output, NULL);
        if (!inputBytes || !outputBytes) return 1;

        int ret = unicodefpe_decrypt(
            (UnicodeFPECipherHandle)handle,
            (const char*)inputBytes, (size_t)inputLen,
            (char*)outputBytes, (size_t)outputCapacity
        );

        env->ReleaseByteArrayElements(output, outputBytes, 0);
        env->ReleaseByteArrayElements(input, inputBytes, JNI_ABORT);

        return ret;
    }

    JNIEXPORT void JNICALL Java_net_todorovich_fpe_jdbc_UnicodeFPECipherJNI_destroy(JNIEnv* env, jobject obj, jlong handle) {
        unicodefpe_destroy((UnicodeFPECipherHandle)handle);
    }

} // extern "C"
