// AclasLoader.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "AclasLoader.h"


#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CoInitialize(NULL);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		CoUninitialize();
		break;
	}
	return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

void _setObjectAddress(JNIEnv *env, jobject self, void* addr)
{
	jfieldID idField = env->GetFieldID(env->GetObjectClass(self), "addr", "J");
	env->SetLongField(self, idField, (jlong)addr);
}

IAClasOLEDriver* _getObjectAddress(JNIEnv* env, jobject self)
{
	jfieldID idField = env->GetFieldID(env->GetObjectClass(self), "address", "J");
	return (IAClasOLEDriver*)env->GetLongField(self, idField);
}

void _setErrorFields(JNIEnv *env, jobject self, HRESULT hr)
{
	jfieldID idField = env->GetFieldID(env->GetObjectClass(self), "result", "J");
	env->SetLongField(self, idField, (jlong)hr);

	IAClasOLEDriverPtr spDriver(_getObjectAddress(env, self));
	if (spDriver && hr == 0) {
		VARIANT_BOOL vbState = VARIANT_TRUE;
		spDriver->get_LastCmdOk(&vbState);
		if (VARIANT_FALSE == vbState) {
			jfieldID idField = env->GetFieldID(env->GetObjectClass(self), "errorNumber", "I");
			LONG errorno = 0L;
			spDriver->get_LastError(&errorno);
			env->SetIntField(self, idField, errorno);

			bstr_t bstrMsg;
			spDriver->get_LastErrorMessage(bstrMsg.GetAddress());
			idField = env->GetFieldID(env->GetObjectClass(self), "errorMessage", "Ljava/lang/String;");
			jstring jsMsg = BSTR2JSTR(env, bstrMsg);
			env->SetObjectField(self, idField, jsMsg);
		}
	}
}

extern "C"
JNIEXPORT jboolean JNICALL Java_com_onegolabs_wamanager_scales_aclas_AclasLoader_connectToScales
(JNIEnv *env, jobject self, jstring ipAddress)
{
	IAClasOLEDriverPtr spDriver;
	HRESULT result = spDriver.CreateInstance(L"AclasDriver.AclasOLEDriver");
	if (FAILED(result)) {
		_setErrorFields(env, self, result);
		return (jboolean)0;
	}

	JSTR2BSTR(env, bstrIP, ipAddress);
	result = spDriver->put_IPAddress(bstrIP);
	if (FAILED(result)) {
		_setErrorFields(env, self, result);
		return (jboolean)0;
	}

	spDriver->InitBufferPLU();
	spDriver->InitBufferMessages();

	_setErrorFields(env, self, 0L);
	_setObjectAddress(env, self, spDriver.Detach());
	return (jboolean)1;
}

extern "C"
JNIEXPORT jboolean JNICALL Java_com_onegolabs_wamanager_scales_aclas_AclasLoader_clearAllPLUAndMessages
(JNIEnv *env, jobject self)
{
	IAClasOLEDriverPtr spDriver(_getObjectAddress(env, self));
	HRESULT hr = spDriver->ClearAllPLUAndMessages();
	if (FAILED(hr)) {
		_setErrorFields(env, self, hr);
		return (jboolean)0;
	}

	_setErrorFields(env, self, 0L);
	return (jboolean)1;
}

extern "C"
JNIEXPORT jint JNICALL Java_com_onegolabs_wamanager_scales_aclas_AclasLoader_sendArticleToScales
(JNIEnv *env, jobject self, jint plu, jint code, jstring name, jdouble price, jint life, jint unit, jint barcode, jint message, jint label, jint discount)
{
	jint ret = -1;
	HRESULT result;

	IAClasOLEDriverPtr spDriver(_getObjectAddress(env, self));
	if (spDriver) {
		result = spDriver->AddPLUToBuffer();
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		result = spDriver->put_PLU_Number(plu);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		result = spDriver->put_PLU_AddCode(code);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		result = spDriver->put_PLU_Code(code);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		JSTR2BSTR(env, bstrName, name);
		result = spDriver->put_PLU_Name(bstrName);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		result = spDriver->put_PLU_WeightUnit(unit);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		result = spDriver->put_PLU_Price(price);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		result = spDriver->put_PLU_ShlefTime(life);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		result = spDriver->put_PLU_BarCodeType(barcode);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		// TODO: HACK: hardcoded department!
		result = spDriver->put_PLU_Dep(27);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		result = spDriver->put_PLU_Rebate(discount);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		if (message > 0) {
			result = spDriver->put_PLU_Message(message);
			if (FAILED(result)) {
				_setErrorFields(env, self, result);
				return -1;
			}
		}

		result = spDriver->get_PLU_CurrentNo(&ret);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}

		result = spDriver->put_PLU_Label(label);
		if (FAILED(result)) {
			_setErrorFields(env, self, result);
			return -1;
		}
	}
	return ret;
}

extern "C"
JNIEXPORT jint JNICALL Java_com_onegolabs_wamanager_scales_aclas_AclasLoader_addMessage
(JNIEnv *env, jobject self)
{
	IAClasOLEDriverPtr spDriver(_getObjectAddress(env, self));
	HRESULT result = spDriver->AddMessageToBuffer();
	if (FAILED(result)) {
		_setErrorFields(env, self, result);
		return -1;
	}

	LONG lMsgNo = 0L;
	result = spDriver->get_Message_CurrentNo(&lMsgNo);
	if (FAILED(result)) {
		_setErrorFields(env, self, result);
		return -1;
	}

	return lMsgNo;
}

extern "C"
JNIEXPORT jboolean JNICALL Java_com_onegolabs_wamanager_scales_aclas_AclasLoader_setMessageLine
(JNIEnv *env, jobject self, jint line, jstring text)
{
	IAClasOLEDriverPtr spDriver(_getObjectAddress(env, self));
	HRESULT result = 0L;

	JSTR2BSTR(env, bstrText, text);

	switch (line) {
	case 0:
		result = spDriver->put_Message_Line1(bstrText);
		break;
	case 1:
		result = spDriver->put_Message_Line2(bstrText);
		break;
	case 2:
		result = spDriver->put_Message_Line3(bstrText);
		break;
	case 3:
		result = spDriver->put_Message_Line4(bstrText);
		break;
	case 4:
		result = spDriver->put_Message_Line5(bstrText);
		break;
	case 5:
		result = spDriver->put_Message_Line6(bstrText);
		break;
	case 6:
		result = spDriver->put_Message_Line7(bstrText);
		break;
	case 7:
		result = spDriver->put_Message_Line8(bstrText);
		break;
	case 8:
		result = spDriver->put_Message_Line9(bstrText);
		break;
	case 9:
		result = spDriver->put_Message_Line10(bstrText);
		break;
	case 10:
		result = spDriver->put_Message_Line11(bstrText);
		break;
	case 11:
		result = spDriver->put_Message_Line12(bstrText);
		break;
	case 12:
		result = spDriver->put_Message_Line13(bstrText);
		break;
	case 13:
		result = spDriver->put_Message_Line14(bstrText);
		break;
	case 14:
		result = spDriver->put_Message_Line15(bstrText);
		break;
	case 15:
		result = spDriver->put_Message_Line16(bstrText);
		break;
	}

	if (FAILED(result)) {
		_setErrorFields(env, self, result);
		return (jboolean)0;
	}

	return (jboolean)1;
}

extern "C"
JNIEXPORT jboolean JNICALL Java_com_onegolabs_wamanager_scales_aclas_AclasLoader_flushBuffers
(JNIEnv *env, jobject self)
{
	IAClasOLEDriverPtr spDriver(_getObjectAddress(env, self));
	HRESULT result;

	if (spDriver) {
		long lCount = 0L;
		spDriver->get_MessageCount(&lCount);
		if (lCount > 0L) {
			result = spDriver->SetAllMessages();
			if (FAILED(result)) {
				_setErrorFields(env, self, result);
				return (jboolean)0;
			}

			VARIANT_BOOL vbState = VARIANT_TRUE;
			spDriver->get_LastCmdOk(&vbState);
			if (VARIANT_FALSE == vbState) {
				_setErrorFields(env, self, 0L);
				return (jboolean)0;
			}
		}

		lCount = 0L;
		spDriver->get_PLUCount(&lCount);
		if (lCount > 0L) {
			result = spDriver->SetAllPLU();
			if (FAILED(result)) {
				_setErrorFields(env, self, result);
				return (jboolean)0;
			}

			VARIANT_BOOL vbState = VARIANT_TRUE;
			spDriver->get_LastCmdOk(&vbState);
			if (VARIANT_FALSE == vbState) {
				_setErrorFields(env, self, 0L);
				return (jboolean)0;
			}
		}
	}

	return (jboolean)1;
}

extern "C"
JNIEXPORT jboolean JNICALL Java_com_onegolabs_wamanager_scales_aclas_AclasLoader_disconnectFromScales
(JNIEnv *env, jobject self)
{
	IAClasOLEDriverPtr spDriver(_getObjectAddress(env, self));
	if (spDriver) {
		spDriver.Release();
	}
	return (jboolean)1;
}
