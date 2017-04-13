// Fill out your copyright notice in the Description page of Project Settings.

#include "SpeechRecognizer.h"
#include "FlytekVoiceSDK.h"
#include "ThreadClass.h"
#include "ScopeLock.h"


//const FString LoginParams = TEXT("appid = 58d087b0, work_dir = .");
//const FString Session_Begin_Params = TEXT("sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = utf-8");

//DEFINE_LOG_CATEGORY(LogFlytekVoiceSDK);

#define SPEECH_THREAD TEXT("SpeechThread")

//const FString Session_Begin_Params = TEXT("sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = utf-8");

USpeechRecognizer* pSpeechRecognizer = nullptr;

extern "C"
{
	void OnSpeechResult(const char* result, char is_last)
	{
		if (pSpeechRecognizer)
		{
			pSpeechRecognizer->OnSpeechRecResult(result, is_last);
		}	
	}

	void OnSpeechBeginResult()
	{
		if (pSpeechRecognizer)
		{
			pSpeechRecognizer->OnSpeechRecBegin();
		}
	}

	void OnSpeechEndResult(int reason)
	{
		if (pSpeechRecognizer)
		{
			pSpeechRecognizer->OnSpeechRecEnd(reason);
		}
	}
}



USpeechRecognizer::USpeechRecognizer()
{
	pSpeechRecognizer = this;
}
USpeechRecognizer::~USpeechRecognizer()
{

}

void USpeechRecognizer::Tick(float DeltaTime)
{
	FScopeLock ScopeLock1(&AccessLock);

	if (SpeechThread.IsValid())
	{
		if (ErrorResult != -1)
		{
			switch (SpeechThread->GetThreadState())
			{
			case ES_NULL:
				break;
			case ES_LOGIN:
				if (ErrorResult == 0)
				{
					bLoginSuccessful = true;
					HandleOnLoginResult();
					UE_LOG(LogFlytekVoiceSDK, Log, TEXT("VoiceSDKLogin Successful ! "))
				}
				else
				{
					bLoginSuccessful = false;
					UE_LOG(LogFlytekVoiceSDK, Error, TEXT("VoiceSDKLogin Faild ! Error code : %d"), ErrorResult)
				}			
				break;
			case ES_LOGOUT:
				if (ErrorResult == 0)
				{
					UE_LOG(LogFlytekVoiceSDK, Log, TEXT("VoiceSDKLogout Successful ! "))
				}
				else
				{
					UE_LOG(LogFlytekVoiceSDK, Error, TEXT("VoiceSDKLogout Faild ! Error code : %d"), ErrorResult)
				}
				break;
			case ES_INIT:
				if (ErrorResult == 0)
				{
					UE_LOG(LogFlytekVoiceSDK, Log, TEXT("VoiceSDK Init Successful ! "))
				}
				else
				{
					UE_LOG(LogFlytekVoiceSDK, Error, TEXT("VoiceSDK Init Faild ! Error code : %d"), ErrorResult)
				}
				break;
			case ES_UNINIT:
				break;
			case ES_STARTLISTENING:
				break;
			case ES_STOPLISTENING:
				break;
			default:
				break;
			}



			
			ErrorResult = -1;
			//HandleOnLoginResult();
		}
	}

}
bool USpeechRecognizer::IsTickable() const
{
	return true;
}

TStatId USpeechRecognizer::GetStatId() const
{
	return TStatId();
}

void USpeechRecognizer::HandleOnLoginResult()
{

}
void USpeechRecognizer::SpeechRecLoginRequest(const FString& UserName, const FString& Password, const FString& Params)
{
	if (bLoginSuccessful)
	{
		UE_LOG(LogFlytekVoiceSDK, Log, TEXT("VoiceSDK has already Login Successful ! "))
		return;
	}
	if (SpeechThread.IsValid())
	{
		SpeechThread->Reset();
		SpeechThread->InitLoginThread(this, &USpeechRecognizer::CallSRLogin, SPEECH_THREAD, UserName, Password, Params, ES_LOGIN);
		SpeechThread->Run();
	}
	else
	{
		SpeechThread = MakeShareable(new FThreadClass(this, &USpeechRecognizer::CallSRLogin, SPEECH_THREAD, UserName, Password, Params, ES_LOGIN));
		SpeechThread->Run();
	}
	
	
}
int32 USpeechRecognizer::CallSRLogin(const FString& UserName, const FString& Password, const FString& Params) 
{ 
	FScopeLock ScopeLock(&AccessLock);
	ErrorResult = sr_login(TCHAR_TO_ANSI(*UserName), TCHAR_TO_ANSI(*Password), TCHAR_TO_ANSI(*Params));
	return ErrorResult;
}

void USpeechRecognizer::SpeechRecLogoutRequest()
{
	if (SpeechThread.IsValid())
	{
		SpeechThread->Reset();
		SpeechThread->InitLogoutThread(this, &USpeechRecognizer::CallSRLogout, SPEECH_THREAD, ES_LOGOUT);
		SpeechThread->Run();
	}
	else
	{
		SpeechThread = MakeShareable(new FThreadClass(this, &USpeechRecognizer::CallSRLogout, SPEECH_THREAD, ES_LOGOUT));
		SpeechThread->Run();
	}
}
void USpeechRecognizer::SpeechRecInitRequest()
{
	if (SpeechThread.IsValid())
	{
		SpeechThread->Reset();
		SpeechThread->InitSpeechInitThread(this, &USpeechRecognizer::CallSRInit, SPEECH_THREAD, ES_INIT);
		SpeechThread->Run();
	}
	else
	{
		SpeechThread = MakeShareable(new FThreadClass(this, &USpeechRecognizer::CallSRInit, SPEECH_THREAD, ES_INIT));
		SpeechThread->Run();
	}

}
void USpeechRecognizer::CallSRLogout()
{
	FScopeLock ScopeLock1(&AccessLock);
	ErrorResult = sr_logout();
}

void USpeechRecognizer::CallSRInit()
{
	RecNotifier = {
		OnSpeechResult,
		OnSpeechBeginResult,
		OnSpeechEndResult
	};
	FScopeLock ScopeLock1(&AccessLock);
	ErrorResult = sr_init(&SpeechRec, TCHAR_TO_ANSI(*Session_Begin_Params), SR_MIC, DEFAULT_INPUT_DEVID, &RecNotifier);
}
/*
void USpeechRecognizer::SpeechRecLogout()
{
	auto Result = sr_logout();
	if (Result == 0)
	{
		UE_LOG(LogFlytekVoiceSDK, Log, TEXT("VoiceSDKLogout Successful ! "))
	}
	else
	{
		UE_LOG(LogFlytekVoiceSDK, Error, TEXT("VoiceSDKLogout Faild ! Error code : %d"), Result)
	}
}

int32 USpeechRecognizer::SpeechRecInit()
{
	RecNotifier = {
		OnResult,
		OnSpeechBegin,
		OnSpeechEnd
	};
	int32 ErrorCode = 0;//= sr_init(&SpeechRec, TCHAR_TO_ANSI(*Session_Begin_Params), SR_MIC, DEFAULT_INPUT_DEVID, &RecNotifier);
	return ErrorCode;
}
void USpeechRecognizer::SpeechRecUninit()
{
	if (bInitSuccessful)
	{
		sr_uninit(&SpeechRec);
	}
	bInitSuccessful = false;
}
void USpeechRecognizer::SpeechRecStartListening()
{
	//	ThreadManager::Create(this, &FFlytekVoiceSDKModule::StartListening, TEXT("StartListeningThread"));

	if (bInitSuccessful)
	{
		int32 ErrorCode = sr_start_listening(&SpeechRec);
		if (ErrorCode == 0)
		{
			UE_LOG(LogFlytekVoiceSDK, Log, TEXT("Start Speech!"))
		}
		else
		{
			UE_LOG(LogFlytekVoiceSDK, Log, TEXT("Start Speech faild! Error Code :%d"), ErrorCode)
		}
	}
	else
	{
		UE_LOG(LogFlytekVoiceSDK, Log, TEXT("Speek Recognizer uninitialized"))
	}
}

void USpeechRecognizer::SpeechRecStopListening()
{
	if (bInitSuccessful)
	{
		int32 ErrorCode = sr_stop_listening(&SpeechRec);
		if (ErrorCode == 0)
		{
			UE_LOG(LogFlytekVoiceSDK, Log, TEXT("Stop Speech!"))
		}
		else
		{
			UE_LOG(LogFlytekVoiceSDK, Log, TEXT("Stop Speech faild! Error Code :%d"), ErrorCode)
		}
	}
	else
	{
		UE_LOG(LogFlytekVoiceSDK, Log, TEXT("Speek Recognizer uninitialized"))
	}
}
int32 USpeechRecognizer::SpeechRecWriteAudioData()
{
	char data;
	int len = 0;
	return sr_write_audio_data(&SpeechRec, &data, len);
}
*/
void USpeechRecognizer::OnSpeechRecResult(const char* result, char is_last)
{
	FString SpeechResultStr = UTF8_TO_TCHAR(result);
	if (bSpeeking)
	{
		if (is_last)
		{
			bSpeeking = false;
			UE_LOG(LogFlytekVoiceSDK, Log, TEXT("Speeking Last words"), *SpeechResultStr)

		}
		UE_LOG(LogFlytekVoiceSDK, Log, TEXT("Speeking String :[%s]"), *SpeechResultStr)
	}
}
void USpeechRecognizer::OnSpeechRecBegin()
{
	bSpeeking = true;
	UE_LOG(LogFlytekVoiceSDK, Log, TEXT("OnSpeechRecBegin"))
}
void USpeechRecognizer::OnSpeechRecEnd(int reason)
{
	if (reason == END_REASON_VAD_DETECT)
	{
		bSpeeking = false;
		UE_LOG(LogFlytekVoiceSDK, Log, TEXT("Speeking Done"))
	}
	else
	{
		bSpeeking = false;
		UE_LOG(LogFlytekVoiceSDK, Error, TEXT("On Speech Recognizer Error : %d"), reason)
	}

}

