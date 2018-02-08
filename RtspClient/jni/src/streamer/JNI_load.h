#ifndef __JNI_LOAD___
#define __JNI_LOAD___

#include <jni.h>

//global java vm
extern JavaVM *g_JVM;

#ifdef __cplusplus

#define JNI_ATTACH_JVM_WITH_NAME(jni_env , thread_name ) \
			JNIEnv *jenv; \
			JavaVMAttachArgs args; \
			args.version = JNI_VERSION_1_4;  \
			args.name = thread_name ; \
			args.group = NULL; \
			int status = g_JVM->GetEnv((void **)&jenv, JNI_VERSION_1_4); \
			jint result = g_JVM->AttachCurrentThread(&jni_env, &args);

#define JNI_ATTACH_JVM(jni_env) \
			JNIEnv *jenv;\
			 int status = g_JVM->GetEnv((void **)&jenv, JNI_VERSION_1_4); \
			jint result = g_JVM->AttachCurrentThread(&jni_env, NULL);

#define JNI_DETACH_JVM(jni_env) 				if( jni_env ) { g_JVM->DetachCurrentThread(); jni_env = 0;}

#define JNI_GET_STATIC_METHOD(type)				g_static_method[(type)]
#define JNI_SET_STATIC_METHOD(type, method)     g_static_method[(type)] = (method)

#define JNI_FINDCLASS(name)						env->FindClass(name);

#define JNI_GET_FIELD_OBJ_ID(jclazz, name)		env->GetFieldID(jclazz, (const char*)name,  "Ljava/lang/Object;")

#define JNI_GET_FIELD_STR_ID(jclazz, name)		env->GetFieldID(jclazz, (const char*)name,  "Ljava/lang/String;")

#define JNI_GET_FIELD_INT_ID(jclazz, name)		env->GetFieldID(jclazz, (const char*)name,  "I")

#define JNI_GET_FIELD_BOOLEAN_ID(jclazz, name)  env->GetFieldID(jclazz, (const char*)name,  "Z")

#define JNI_GET_FIELD_LONG_ID(jclazz, name)		env->GetFieldID(jclazz, (const char*)name,  "J")

#define JNI_GET_FIELD_DOUBLE_ID(jclazz, name)	env->GetFieldID(jclazz, (const char*)name,  "D")

#define JNI_GET_FIELD_BYTE_ID(jclazz, name)		env->GetFieldID(jclazz, (const char*)name,  "B")

#define JNI_GET_METHOD_ID(jclazz, name, sig)	env->GetMethodID(jclazz, name, sig)

#define JNI_GET_OBJ_FIELD(obj, methodId)		env->GetObjectField(obj, methodId)

#define JNI_GET_INT_FIELD(obj, methodId)		env->GetIntField(obj, methodId)

#define JNI_GET_LONG_FILED(obj, methodId)		env->GetLongField(obj, methodId)

#define JNI_GET_DOUBLE_FILED(obj, methodId)		env->GetDoubleField(obj, methodId)

#define JNI_GET_BYTE_FILED(obj, methodId)		env->GetByteField(obj, methodId)

#define JNI_GET_BOOLEAN_FIELD(obj, methodId)	env->GetBooleanField(obj, methodId)

#define JNI_GET_UTF_STR(str, str_obj, type)		if(NULL != str_obj) {\
													str = (type)env->GetStringUTFChars(str_obj, NULL);\
												} else {\
													str = (type)NULL;\
												}

#define JNI_GET_UTF_CHAR(str, str_obj)  		JNI_GET_UTF_STR(str, str_obj, char*);

#define JNI_RELEASE_STR_STR(str, str_obj)		if( NULL != str_obj && NULL != str) {\
													env->ReleaseStringUTFChars(str_obj, (const char*)str);\
												}

#define JNI_GET_BYTE_ARRAY(b, array)			if( NULL != array) {\
													b = (INT8S*)env->GetByteArrayElements(array, (jboolean*)NULL);\
												} else {\
														b = NULL;\
												}

#define JNI_RELEASE_BYTE_ARRAY(b, array)		if( NULL != array) {\
													env->ReleaseByteArrayElements(array, (jbyte*)b, JNI_ABORT);\
												}

#define JNI_DELETE_LOCAL_REF(obj)				if( NULL != obj) {\
													env->DeleteLocalRef(obj);\
												}

#else

#define JNI_ATTACH_JVM(jni_env) \
			JNIEnv *jenv;\
			 int status = g_JVM->GetEnv((void **)&jenv, JNI_VERSION_1_4); \
			jint result = g_JVM->AttachCurrentThread(&jni_env, NULL);

#define JNI_DETACH_JVM(jni_env) 				if( jni_env ) { g_JVM->DetachCurrentThread(); jni_env = 0;}

#define JNI_GET_STATIC_METHOD(type)				g_static_method[(type)]
#define JNI_SET_STATIC_METHOD(type, method)     g_static_method[(type)] = (method)

#define JNI_FINDCLASS(name)						(*env)->FindClass(name);

#define JNI_GET_FIELD_OBJ_ID(jclazz, name)		(*env)->GetFieldID(jclazz, (const char*)name,  "Ljava/lang/Object;")

#define JNI_GET_FIELD_STR_ID(jclazz, name)		(*env)->GetFieldID(jclazz, (const char*)name,  "Ljava/lang/String;")

#define JNI_GET_FIELD_INT_ID(jclazz, name)		(*env)->GetFieldID(jclazz, (const char*)name,  "I")

#define JNI_GET_FIELD_BOOLEAN_ID(jclazz, name)  (*env)->GetFieldID(jclazz, (const char*)name,  "Z")

#define JNI_GET_FIELD_LONG_ID(jclazz, name)		(*env)->GetFieldID(jclazz, (const char*)name,  "J")

#define JNI_GET_FIELD_DOUBLE_ID(jclazz, name)	(*env)->GetFieldID(jclazz, (const char*)name,  "D")

#define JNI_GET_FIELD_BYTE_ID(jclazz, name)		(*env)->GetFieldID(jclazz, (const char*)name,  "B")

#define JNI_GET_METHOD_ID(jclazz, name, sig)	(*env)->GetMethodID(jclazz, name, sig)

#define JNI_GET_OBJ_FIELD(obj, methodId)		(*env)->GetObjectField(obj, methodId)

#define JNI_GET_INT_FIELD(obj, methodId)		(*env)->GetIntField(obj, methodId)

#define JNI_GET_LONG_FILED(obj, methodId)		(*env)->GetLongField(obj, methodId)

#define JNI_GET_DOUBLE_FILED(obj, methodId)		(*env)->GetDoubleField(obj, methodId)

#define JNI_GET_BYTE_FILED(obj, methodId)		(*env)->GetByteField(obj, methodId)

#define JNI_GET_BOOLEAN_FIELD(obj, methodId)	(*env)->GetBooleanField(obj, methodId)

#define JNI_GET_UTF_STR(str, str_obj, type)		if(NULL != str_obj) {\
													str = (type)(*env)->GetStringUTFChars(str_obj, NULL);\
												} else {\
													str = (type)NULL;\
												}

#define JNI_GET_UTF_CHAR(str, str_obj)  		JNI_GET_UTF_STR(str, str_obj, char*);

#define JNI_RELEASE_STR_STR(str, str_obj)		if( NULL != str_obj && NULL != str) {\
													(*env)->ReleaseStringUTFChars(str_obj, (const char*)str);\
												}

#define JNI_GET_BYTE_ARRAY(b, array)			if( NULL != array) {\
													b = (INT8S*)(*env)->GetByteArrayElements(array, (jboolean*)NULL);\
												} else {\
														b = NULL;\
												}

#define JNI_RELEASE_BYTE_ARRAY(b, array)		if( NULL != array) {\
													(*env)->ReleaseByteArrayElements(array, (jbyte*)b, JNI_ABORT);\
												}

#define JNI_DELETE_LOCAL_REF(obj)				if( NULL != obj) {\
													(*env)->DeleteLocalRef(obj);\
												}
#endif

#endif//__JNI_LOAD___
