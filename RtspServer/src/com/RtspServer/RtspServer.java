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
 */
package com.RtspServer;

import android.app.Activity;
import android.view.View;
import android.widget.Button;
import android.os.Bundle;
 
public class RtspServer extends Activity
{
	private int m_clickCount = 0;
	private RtspServerAPI rtspServerAPI = new RtspServerAPI();
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        /* Create a Button and set its content.
         * the text is retrieved by calling a native
         * function.
         */
        final Button  button = new Button(this);
		button.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				m_clickCount++;
				if(m_clickCount%5==1)
				{
					rtspServerAPI.RtspCreate();
					button.setText("RtspCreate Called!");
				}
				else if(m_clickCount%5==2)
				{
					rtspServerAPI.RtspStart();
					button.setText("RtspStart Called!");
				}
				else if(m_clickCount%5==3)
				{
					rtspServerAPI.RtspStop();
					button.setText("RtspStop Called!");
				}
				else if(m_clickCount%5==4)
				{
					rtspServerAPI.RtspDestory();
					button.setText("RtspDestory Called!");
				}
				else if(m_clickCount%5==0)
				{
					button.setText(rtspServerAPI.stringFromJNI());
				}
				//button.setText( stringFromJNI());
			}
		});
        //button.setText( stringFromJNI() );
        setContentView(button);
    }

    /* A native method that is implemented by the
     * 'RtspServer' native library, which is packaged
     * with this application.
     */


    /* this is used to load the 'RtspServer' library on application
     * startup. The library has already been unpacked into
     * /data/data/com.RtspServer/lib/RtspServer.so at
     * installation time by the package manager.
     */

}
