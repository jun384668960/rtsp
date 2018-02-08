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
#include <stdio.h>

int s_ButtonPressCounter = 0;

jstring
Java_com_visualgdb_example_AndroidLive_AndroidLive_stringFromJNI( JNIEnv* env,
                                                  jobject thiz )
{
	char szBuf[512];
	sprintf(szBuf, "You have pressed this huge button %d times", s_ButtonPressCounter++);
  
	jstring str = (*env)->NewStringUTF(env, szBuf);
	return str;
}
