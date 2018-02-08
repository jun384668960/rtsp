/*
 * Created by VisualGDB. Based on hello-jni example.

 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <string.h>
#include <jni.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <android/log.h>
#include "RtspSvr.hh"

static int s_ButtonPressCounter = 0;

jstring
Java_com_RtspServer_RtspServerAPI_stringFromJNI( JNIEnv* env,
	jobject thiz)
{
	char szBuf[512];
	sprintf(szBuf, "Java_com_AndroidTestApp_AndroidTestApp_stringFromJNI\nYou have pressed this huge button %d times", s_ButtonPressCounter++);
  
	jstring str = env->NewStringUTF(szBuf);
	return str;
}

JNIEXPORT void JNICALL Java_com_RtspServer_RtspServerAPI_Create(JNIEnv* env, jobject thiz)
{
	__android_log_print(ANDROID_LOG_INFO, "rtsp_jni", "Java_com_RtspServer_RtspServer_Create");	
	CRtspServer::GetInstance()->Create();
}

JNIEXPORT void JNICALL Java_com_RtspServer_RtspServerAPI_Destory(JNIEnv* env, jobject thiz)
{
	CRtspServer::GetInstance()->Destory();
}

JNIEXPORT void JNICALL Java_com_RtspServer_RtspServerAPI_Start(JNIEnv* env, jobject thiz)
{
	__android_log_print(ANDROID_LOG_INFO, "rtsp_jni", "Java_com_RtspServer_RtspServer_Start");	
	CRtspServer::GetInstance()->Start();
}

JNIEXPORT void JNICALL Java_com_RtspServer_RtspServerAPI_Stop(JNIEnv* env, jobject thiz)
{
	CRtspServer::GetInstance()->Stop();
}

JNIEXPORT void JNICALL Java_com_RtspServer_RtspServerAPI_Restart(JNIEnv* env, jobject thiz)
{
	CRtspServer::GetInstance()->Restart();
}

JNIEXPORT void JNICALL Java_com_RtspServer_RtspServerAPI_SourcePush(JNIEnv* env, jobject thiz, jbyteArray data, jlong len, jlong pts, jint flag)
{
	jbyte* szStr = env->GetByteArrayElements(data, NULL);
	    //然后去用szStr吧，就是对jbyteArray szLics的使用

	CRtspServer::GetInstance()->LiveSourcePush((char*)szStr,len,pts,flag);
		
	env->ReleaseByteArrayElements(data, szStr, 0);
}

JNIEXPORT void JNICALL Java_com_RtspServer_RtspServerAPI_SourceSync(JNIEnv* env, jobject thiz, jint flag)
{
	CRtspServer::GetInstance()->LiveSourceSync(flag);
}

static JNINativeMethod gMethods[] = {
	{ "stringFromJNI", "()Ljava/lang/String;", (void *)Java_com_RtspServer_RtspServerAPI_stringFromJNI },
	{ "RtspCreate", "()V", (void *)Java_com_RtspServer_RtspServerAPI_Create },
	{ "RtspDestory", "()V", (void *)Java_com_RtspServer_RtspServerAPI_Destory },
	{ "RtspStart", "()V", (void *)Java_com_RtspServer_RtspServerAPI_Start },
	{ "RtspStop", "()V", (void *)Java_com_RtspServer_RtspServerAPI_Stop },
	{ "RtspRestart", "()V", (void *)Java_com_RtspServer_RtspServerAPI_Restart },
	{ "RtspSourcePush", "([BJJI)V", (void *)Java_com_RtspServer_RtspServerAPI_SourcePush },
	{ "RtspSourceSync", "(I)V", (void *)Java_com_RtspServer_RtspServerAPI_SourceSync },
};

#define RTSPSERVER_JAVA_CLASS_PATH "com/RtspServer/RtspServerAPI"
static int register_android_RtspServer(JNIEnv *env)
{
	jclass clazz = env->FindClass(RTSPSERVER_JAVA_CLASS_PATH);
	if (clazz == NULL) {
		return -1;
	}
	if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
		return -2;
	}

	return 0;
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	JNIEnv *env;

	if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
		return -1;
	}
	
	assert(env != NULL);
	if (register_android_RtspServer(env) < 0) {
		printf("register_android_test_hello error.\n");
		return -1;
	}
	return JNI_VERSION_1_4;
}