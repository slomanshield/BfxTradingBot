#include "BfxLibrary.h"

BfxLibrary::BfxLibrary()
{
	apiKey = "";
	secretKey = "";
	responseData = "";
	currentPrice = -1;

	webSocketTradeChannel.setUrl(WebSocketURL);
	webSocketTradeChannel.setOnMessageCallback(recieveCallBackTradeChannel);
	webSocketTradeChannel.disableAutomaticReconnection();
	webSocketTradeChannel.setUsrPtr((void*)this);
	webSocketTradeChannel.setPingInterval(5);
	webSocketTradeChannel.setPingTimeout(10);

	webSocketAuth.setUrl(WebSocketURL);
	webSocketAuth.setOnMessageCallback(recieveCallBackAuth);
	webSocketAuth.disableAutomaticReconnection();
	webSocketAuth.setUsrPtr((void*)this);
	webSocketAuth.setPingInterval(5);
	webSocketAuth.setPingTimeout(10);

	auth_channel_id = -1;

	curl = NULL;
}

BfxLibrary::~BfxLibrary()
{
	if (curl != NULL)
		curl_easy_cleanup(curl);
}

void BfxLibrary::InitKeys(char * apiKey, char * secretKey)
{
	this->apiKey = apiKey;
	this->secretKey = secretKey;


}

void BfxLibrary::InitCurl()
{

	if (curl == NULL)
		curl = curl_easy_init();
	else
		curl_easy_reset(curl);


	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	}
}

int BfxLibrary::ParseResponseData(Document * out)
{
	int cc = 0;

	ParseResult res = out->Parse< kParseStopWhenDoneFlag>(responseData.c_str());

	if (res.IsError())
		cc = MALFORMED_JSON_FROM_BFX;

	return cc;
}

std::string BfxLibrary::GetBase64(std::string input)
{
	std::string output;
	BIO *bio, *b64;
	BUF_MEM *bufferPtr;

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
	BIO_write(bio, input.c_str(), input.length());
	BIO_flush(bio);
	BIO_get_mem_ptr(bio, &bufferPtr);

	output.append((*bufferPtr).data, (*bufferPtr).length);
	BIO_free_all(bio);

	return output;
}



std::string BfxLibrary::GetSignature(std::string* payload)
{
	std::string digest_hex;
	char hex[2 + FOR_NULL] = { 0 };
	unsigned char * digest;

	digest = HMAC(EVP_sha384(), secretKey.c_str(), secretKey.length(), (unsigned char *)payload->c_str(), payload->length(), NULL, NULL);

	for (int i = 0; i < SHA384_DIGEST_LENGTH; i++)
	{
		sprintf(hex, "%02x", digest[i]);
		digest_hex.append(hex);
	}

	return digest_hex;
}

void BfxLibrary::AddAuthHeaders(CURL* curl, struct curl_slist **chunk, std::string* payload, std::string* signature)
{
	if (curl)
	{
		std::string apiKeyHeaderFormat = "X-BFX-APIKEY: %s";
		std::string payloadHeaderFormat = "X-BFX-PAYLOAD: %s";
		std::string signatureHeaderFormat = "X-BFX-SIGNATURE: %s";

		*chunk = curl_slist_append(*chunk, (string_format(apiKeyHeaderFormat, apiKey.c_str())).c_str());
		*chunk = curl_slist_append(*chunk, (string_format(payloadHeaderFormat, payload->c_str())).c_str());
		*chunk = curl_slist_append(*chunk, (string_format(signatureHeaderFormat, signature->c_str())).c_str());

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *chunk);

	}
}

void BfxLibrary::InitalizeBasePayload(Document * in, char* request)
{
	in->Parse("{ }");
	requestValJson.SetString(request, strlen(request));
	in->AddMember("request", requestValJson, in->GetAllocator());
	nonce = string_format(format_long_int, GET_NONCE);
	nonceValJson.SetString(nonce.c_str(), nonce.length());
	in->AddMember("nonce", nonceValJson, in->GetAllocator());
	responseData = "";//initalize response data
}

void BfxLibrary::CreateOutputPayload(CURL* curl, struct curl_slist **in_chunk)
{
	base64Payload = GetBase64(inputData);
	signature = GetSignature(&base64Payload);
	AddAuthHeaders(curl, in_chunk, &base64Payload, &signature);
}

void BfxLibrary::ExtractCurrentPrice(std::string* str)
{
	Document doc;
	ParseResult parseResult = doc.Parse(str->c_str());

	if (parseResult.IsError() == false && doc.IsArray())
	{
		Value& valArray = doc[2];//get array

		if (valArray.IsArray())
		{
			Value& val = valArray[3];//4th entry in nested array is the trade price
			if(val.IsNumber())
				currentPrice = val.GetDouble();
		}
	}
	else
		currentPrice = -1;
	
}

void BfxLibrary::ExtractPositionSize(std::string* str)
{
	Document doc;
	ParseResult parseResult = doc.Parse(str->c_str());

	if (parseResult.IsError() == false && doc.IsArray())
	{
		if (doc[2].Size() != 0)
		{
			Value& valArray = doc[2];
			for (int i = 0; i < valArray.Size(); i++)
			{
				Value& valArrayPosition = valArray[i];

				//position 1 is symbol position 2 is status position 3 is amount
				if (bfxSymbol.compare(valArrayPosition[0].GetString()) == SUCCESS)
				{
					currentPositionAmount = valArrayPosition[2].GetDouble();
					return;
				}
				else
					currentPositionAmount = 0;
			}
		}
		else
			currentPositionAmount = 0;
	}
	else
		currentPositionAmount = 0;
}

void BfxLibrary::ExtractPositionSizeUpdates(std::string * str)
{
	Document doc;
	ParseResult parseResult = doc.Parse(str->c_str());
	
	if (parseResult.IsError() == false && doc.IsArray())
	{
		Value& valTopArray = doc[2];
		if (valTopArray.Size() != 0)
		{
			
			Value& valArraySymbol = valTopArray[0];
			Value& valArrayAmount = valTopArray[2];

			//position 1 is symbol position 2 is status position 3 is amount
			if (bfxSymbol.compare(valArraySymbol.GetString()) == SUCCESS)
			{
				currentPositionAmount = valArrayAmount.GetDouble();
			}
			
		}
		else
			currentPositionAmount = 0;
	}
	else
		currentPositionAmount = 0;
}

void BfxLibrary::ExtractTradeableBalance(Document * doc, double * balance_out, double* balance_pl_out)
{
	if (doc->IsArray())
	{
		Value& valArray = doc->operator[](2);

		if (valArray.IsArray())
		{
			Value& valArraySymbolData = valArray[2];

			if (valArraySymbolData.IsArray())
			{
				*balance_out = valArraySymbolData[1].GetDouble();
				*balance_pl_out = valArraySymbolData[0].GetDouble();
			}
			else
				*balance_out = -1;
		}
		else
			*balance_out = -1;
	}
	else
		*balance_out = -1;
}

//Sunday == 0, Monday == 1 so on
void BfxLibrary::GetBfxWeeklyTimeSpan(  long long* startTime_out,   long long* endTime_out)
{
	time_t rawTime;
	time_t prevSundayEndTime;
	time_t prevMondayStartTime;
	struct tm * ptm_current;
	struct tm * prevSundayEnd;
	long long timeVal = 0;

	time(&rawTime);

	ptm_current = gmtime(&rawTime);
	

	if (ptm_current->tm_wday >= MONDAY)
		prevSundayEndTime = rawTime - ((ptm_current->tm_wday - SUNDAY) * DAILY_SECS);
	else
		prevSundayEndTime = rawTime - WEEK_SECS; //go back a week because its sunday weekly hasnt finished yet

	prevSundayEnd = gmtime(&prevSundayEndTime);
	prevSundayEnd->tm_hour = 0;
	prevSundayEnd->tm_min = 0;
	prevSundayEnd->tm_sec = 0;

	prevSundayEndTime = mktime(prevSundayEnd) - timezone;
	*endTime_out = prevSundayEndTime;
	*endTime_out *= 1000;
	
	prevMondayStartTime = prevSundayEndTime - (DAILY_SECS * 6);
	*startTime_out = prevMondayStartTime;
	*startTime_out *= 1000;//get it in milli for bfx

}

void BfxLibrary::CalculateWeeklyPivotData(Document* doc, double * pivot_point_out, double * s1_short_out, double * s2_short_out, double * s3_short_out, double * r1_long_out, double * r2_long_out, double * r3_long_out)
{
	double high = 0;
	double low = DBL_MAX;
	double close = 0;
	
	if (doc->IsArray())
	{
		Value& valArray = doc[0];//because its an array within an array just get the whole array first
		int size = valArray.Size();
		for (int i = 0; i < size; i++)
		{
			Value& val = valArray[i];
			if (val.IsArray())
			{
				double local_high = val[3].GetDouble();//high
				double local_low = val[4].GetDouble();//low
				if (i == 0)//start of arry has most recent data
					close = val[2].GetDouble();

				if (local_high > high)
					high = local_high;
				if (local_low < low)
					low = local_low;
			}

		}
	}

	*pivot_point_out = (high + low + close) / 3;
	*s1_short_out = (2 * *pivot_point_out) - high;
	*s2_short_out = *pivot_point_out - (high - low);
	*s3_short_out = low - (2 * (high - *pivot_point_out));
	*r1_long_out = (2 * *pivot_point_out) - low;
	*r2_long_out = *pivot_point_out + (high - low);
	*r3_long_out = high + (2 * (*pivot_point_out - low));

	return;
}

void BfxLibrary::CalculateTimeFramePivotData(Document * doc,unsigned long long timeFrame, double * pivot_point_out, double * s1_short_out, double * s2_short_out, double * s3_short_out, double * r1_long_out, double * r2_long_out, double * r3_long_out)
{
	double high = 0;
	double low = DBL_MAX;
	double close = 0;

	if (doc->IsArray())
	{
		for (int i = 0; i < doc->Size(); i++)
		{
			Value& val = doc[0].operator[](i);
			if (val[0].GetInt64() == timeFrame)
			{
				high = val[3].GetDouble();//high
				low = val[4].GetDouble();//low
				close = val[2].GetDouble();//close
			}	
		}

	}

	*pivot_point_out = (high + low + close) / 3;
	*s1_short_out = (2 * *pivot_point_out) - high;
	*s2_short_out = *pivot_point_out - (high - low);
	*s3_short_out = low - (2 * (high - *pivot_point_out));
	*r1_long_out = (2 * *pivot_point_out) - low;
	*r2_long_out = *pivot_point_out + (high - low);
	*r3_long_out = high + (2 * (*pivot_point_out - low));

	return;
}

void BfxLibrary::FindMinimumOrderSizeSymbol(Document* doc,std::string * symbol, double * minimum_out)
{
	std::string lowerSymbol = "";
	ToLowerString(symbol, &lowerSymbol);
	if (doc->IsArray())
	{
		Value& valArray = doc[0];//because its an array within an array just get the whole array first
		int size = valArray.Size();
		for (int i = 0; i < size; i++)
		{
			Value& val = valArray[i];
			if (val.IsObject())
			{
				if (val["minimum_order_size"].IsString() && val["pair"].IsString())
				{
					if (lowerSymbol.compare(val["pair"].GetString()) == SUCCESS)
					{
						*minimum_out = atof(val["minimum_order_size"].GetString());
						break;
					}
				}
			}
		}
		


	}

	return;

}

int BfxLibrary::GetWeeklyPivot(double * pivot_point_out, double * s1_short_out, double * s2_short_out, double * s3_short_out,
									 double * r1_long_out, double * r2_long_out, double * r3_long_out,std::string* symbol)
{
	int cc = SUCCESS;
	long long startTime = 0;
	long long endTime = 0;
	
	Document doc;

	GetBfxWeeklyTimeSpan(&startTime, &endTime);

	InitCurl();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, (string_format(std::string(GET_DAILY_CANDLES), bfxSymbol.c_str(), startTime, endTime)).c_str());
		responseData = "";//initalize response data
		cc = curl_easy_perform(curl);

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

		if (cc == SUCCESS && response_code == HTTP_OK)
		{
			cc = ParseResponseData(&doc);
		}
		else
			cc = CURL_HTTP_ERROR;
	}
	else
		cc = CURL_NOT_INITALIZED;

	if (cc != SUCCESS)
	{
		LOG_ERROR_CODE(cc);
	}

	if (cc == SUCCESS)
	{
		CalculateWeeklyPivotData(&doc, pivot_point_out, s1_short_out, s2_short_out, s3_short_out,
			r1_long_out, r2_long_out, r3_long_out);
	}


	return cc;
}

int BfxLibrary::GetTimeFramePivot(ConfigTimeFrame candleTimeFrame,unsigned long long timeFrame, double * pivot_point_out, double * s1_short_out, double * s2_short_out, double * s3_short_out, double * r1_long_out, double * r2_long_out, double * r3_long_out, std::string * symbol)
{
	int cc = SUCCESS;
	long long startTime = 0;
	long long endTime = 0;

	Document doc;

	InitCurl();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, (string_format(std::string(GET_TIMEFRAME_CANDLES), time_frame_array[candleTimeFrame],bfxSymbol.c_str())).c_str());
		responseData = "";//initalize response data
		cc = curl_easy_perform(curl);

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

		if (cc == SUCCESS && response_code == HTTP_OK)
		{
			cc = ParseResponseData(&doc);
		}
		else
			cc = CURL_HTTP_ERROR;
	}
	else
		cc = CURL_NOT_INITALIZED;

	if (cc != SUCCESS)
	{
		LOG_ERROR_CODE(cc);
	}

	if (cc == SUCCESS)
	{
		CalculateTimeFramePivotData(&doc, timeFrame, pivot_point_out, s1_short_out, s2_short_out, s3_short_out,
			r1_long_out, r2_long_out, r3_long_out);
	}


	return cc;
}

int BfxLibrary::GetMinimumOrderSize(std::string * symbol, double * minimum_out)
{
	int cc = SUCCESS;
	Document doc;

	InitCurl();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, GET_SYMBOL_DETAILS);
		responseData = "";//initalize response data
		cc = curl_easy_perform(curl);

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

		if (cc == SUCCESS && response_code == HTTP_OK)
		{
			cc = ParseResponseData(&doc);
		}
		else
			cc = CURL_HTTP_ERROR;
	}
	else
		cc = CURL_NOT_INITALIZED;

	if (cc != SUCCESS)
	{
		LOG_ERROR_CODE(cc);
	}

	if (cc == SUCCESS)
	{
		FindMinimumOrderSizeSymbol(&doc, symbol, minimum_out);
	}

	return cc;
}

int BfxLibrary::ConnectWebSocketTradeChannel()
{
	int cc = SUCCESS;
	ix::WebSocketInitResult initResult;
	initResult = webSocketTradeChannel.connect(5000);

	if (initResult.success == false)
	{
		LOG_ERROR(initResult.errorStr);
		cc = WebSocketConnectFailure;
	}
	else
	{
		webSocketTradeChannel.start();
	}

	return cc;
}

int BfxLibrary::DisconnetWebSocketTradeChannel()
{
	webSocketTradeChannel.stop();
	return SUCCESS;
}

int BfxLibrary::DisconnetWebSocketAuth()
{
	webSocketAuth.stop();
	return SUCCESS;
}

int BfxLibrary::SendAuthWebSocket(std::string* symbol)
{
	int cc = SUCCESS;
	bfxSymbol = "t";//init to correct first char
	ToUpperString(symbol, &bfxSymbol);
	if (webSocketAuth.getReadyState() == ReadyState::Open)
	{
		std::string authInput = CreateWebSocketAuthPayload();
		Document doc;

		cc = SendRecieveWebSocketData(&webSocketAuth,&authInput, &responseData);

		if (cc == SUCCESS)
		{
			cc = ParseResponseData(&doc);
		}

		if (cc == SUCCESS)
		{
			if (doc.HasMember("status") && doc["status"].IsString())
			{
				authStatus = doc["status"].GetString();
				if (authStatus.compare("OK") != SUCCESS)
				{
					LOG_ERROR(string_format(format_string_auth, authStatus));
					cc = ERROR_AUTH;
				}
			}

			if (cc == SUCCESS && doc.HasMember("chanId") && doc["chanId"].IsNumber())
			{
				auth_channel_id = doc["chanId"].GetInt();
			}

		}
	}
	else
		cc = WebSocketNotConnected;

	if (cc != SUCCESS)
	{
		LOG_ERROR_CODE(cc);
	}

	return cc;
}

int BfxLibrary::ConnectWebSocketAuth()
{
	int cc = SUCCESS;
	ix::WebSocketInitResult initResult;
	initResult = webSocketAuth.connect(5000);

	if (initResult.success == false)
	{
		LOG_ERROR(initResult.errorStr);
		cc = WebSocketConnectFailure;
	}
	else
	{
		webSocketAuth.start();
	}
	return cc;
}

int BfxLibrary::SubscribeToTradeChannel(std::string* symbol)
{
	int cc = SUCCESS;
	Document doc;
	if (webSocketTradeChannel.getReadyState() == ReadyState::Open)
	{
		inputData = CreateSubscribePayload(symbol);

		cc = SendRecieveWebSocketData(&webSocketTradeChannel,&inputData, &responseData);

		if (cc == SUCCESS)
		{
			cc = ParseResponseData(&doc);
		}

	}
	else
		cc = WebSocketNotConnected;

	if (cc != SUCCESS)
	{
		LOG_ERROR_CODE(cc);
	}

	return cc;
}

double BfxLibrary::GetCurrentPositionAmount()
{
	return currentPositionAmount;
}

int BfxLibrary::GetTradeableBalance(double * balance_out,double* balance_pl_out)
{
	int cc = SUCCESS;
	
	if (webSocketAuth.getReadyState() == ReadyState::Open)
	{
		std::string input = CreateMarginInfoPayload();
		Document doc;

		cc = SendRecieveWebSocketData(&webSocketAuth, &input, &responseData);

		if (cc == SUCCESS)
		{
			cc = ParseResponseData(&doc);
		}

		if (cc == SUCCESS)
		{
			ExtractTradeableBalance(&doc, balance_out, balance_pl_out);
		}
	}
	else
		cc = WebSocketNotConnected;

	if (cc != SUCCESS)
	{
		LOG_ERROR_CODE(cc);
	}

	return cc;
}

int BfxLibrary::CreateClosePosition(double * amount)
{
	int cc = SUCCESS;
	
	if (webSocketTradeChannel.getReadyState() == ReadyState::Open)
	{
		Document doc;
		inputData = CreateOrderPayload(amount,close_position);

		cc = SendRecieveWebSocketData(&webSocketAuth, &inputData, &responseData);

		if (cc == SUCCESS)
		{
			cc = ParseResponseData(&doc);
		}

		if (cc == SUCCESS)
		{
			responseString = doc[2].GetArray().operator[](6).GetString();
			if (responseString.compare("SUCCESS") != 0)
			{
				cc = WebSocketErrorFromBfx;
				LOG_ERROR(doc[2].GetArray().operator[](7).GetString());
			}
		}

	}
	else
		cc = WebSocketNotConnected;

	if (cc != SUCCESS)
	{
		LOG_ERROR_CODE(cc);
	}

	return cc;
}

int BfxLibrary::CreateOrder(double * amount)
{
	int cc = SUCCESS;

	if (webSocketTradeChannel.getReadyState() == ReadyState::Open)
	{
		Document doc;
		inputData = CreateOrderPayload(amount, order);

		cc = SendRecieveWebSocketData(&webSocketAuth, &inputData, &responseData);

		if (cc == SUCCESS)
		{
			cc = ParseResponseData(&doc);
		}

		if (cc == SUCCESS)
		{
			responseString = doc[2].GetArray().operator[](6).GetString();
			if (responseString.compare("SUCCESS") != 0)
			{
				std::string error_string = doc[2].GetArray().operator[](7).GetString();
				if (error_string.find("not enough tradable balance") != -1)
					cc = INSUFFICENT_FUNDS;
				else
					cc = WebSocketErrorFromBfx;
				LOG_ERROR(error_string.c_str());
			}
		}

	}
	else
		cc = WebSocketNotConnected;

	if (cc != SUCCESS)
	{
		LOG_ERROR_CODE(cc);
	}

	return cc;
}

void BfxLibrary::recieveCallBackTradeChannel(WebSocketMessagePtr socketMessage,
	void* objectPtr)
{
	BfxLibrary* bfxObject = (BfxLibrary*)objectPtr;

	if (socketMessage->type == ix::WebSocketMessageType::Message)
	{
		if (socketMessage->str.find("\"subscribed\"") != -1)//we have subsrcibed
		{
			bfxObject->webSocketRecievedData = socketMessage->str.c_str();
			bfxObject->webSocketDataRecieved = true;
		}
		else if (socketMessage->str.find("\"tu\"") != -1 || socketMessage->str.find("\"te\"") != -1)//we have trade data
		{
			bfxObject->ExtractCurrentPrice(&socketMessage->str);
		}
			
	}



}

void BfxLibrary::recieveCallBackAuth(WebSocketMessagePtr socketMessage,
	void* objectPtr)
{
	BfxLibrary* bfxObject = (BfxLibrary*)objectPtr;

	if (socketMessage->type == ix::WebSocketMessageType::Message)
	{
		if (socketMessage->str.find("\"auth\"") != -1)//we have auth data
		{
			bfxObject->webSocketRecievedData = socketMessage->str.c_str();
			bfxObject->webSocketDataRecieved = true;
		}
		else if (socketMessage->str.find("\"miu\"") != -1)//we have margin info
		{
			bfxObject->webSocketRecievedData = socketMessage->str.c_str();
			bfxObject->webSocketDataRecieved = true;
		}
		else if (socketMessage->str.find("\"on-req\"") != -1)//we have margin info
		{
			bfxObject->webSocketRecievedData = socketMessage->str.c_str();
			bfxObject->webSocketDataRecieved = true;
		}
		else if (socketMessage->str.find("\"ps\"") != -1)//we have position data
		{
			bfxObject->ExtractPositionSize(&socketMessage->str);
		}
		else if (socketMessage->str.find("\"pu\"") != -1 || socketMessage->str.find("\"pc\"") != -1)//position update or close
		{
			bfxObject->ExtractPositionSizeUpdates(&socketMessage->str);
		}
	}

}

void BfxLibrary::InitalizeWebSocketPayload(Document * in, int type)
{
	switch (type)
	{
		case subscribe_trading:
		{
			in->Parse("{ \"event\": \"subscribe\",\"channel\": \"trades\",\"symbol\": \"SYMBOL\"}");
			break;
		}
		case order:
		{
			in->Parse("[0,\"on\", null,  { \"type\": \"MARKET\", \"symbol\": \"tBTCUSD\", \"amount\": \"0\"} ]");
			break;
		}
		case close_position:
		{
			in->Parse("[0,\"on\", null,  { \"type\": \"MARKET\", \"symbol\": \"tBTCUSD\", \"amount\": \"0\", \"flags\": 512} ]");//512 means close position any amount works (just set minimal)
			break;
		}
		
	}
	webSocketRecievedData = "";
	responseData = "";
}

std::string BfxLibrary::CreateSubscribePayload(std::string* symbol)
{
	Document doc;
	std::string output = "";
	std::string symbolSubscribe = string_format(std::string("t%s"), symbol->c_str()).c_str();
	InitalizeWebSocketPayload(&doc, subscribe_trading);
	doc["symbol"].SetString(symbolSubscribe.c_str(), symbolSubscribe.length());

	GetJsonString(&doc, &output);
	return output;
}

std::string BfxLibrary::CreateWebSocketAuthPayload()
{
	Document doc;
	std::string out = "";
	unsigned long long nonce_int = GET_NONCE;
	std::string authPayload = string_format(auth_payload, nonce_int);
	signature = GetSignature(&authPayload);
	nonce = string_format(format_long_int, nonce_int);

	nonceValJson.SetString(nonce.c_str(), nonce.length());
	apiKeyValJson.SetString(apiKey.c_str(), apiKey.length());
	sigValJson.SetString(signature.c_str(), signature.length());
	authPayloadValJson.SetString(authPayload.c_str(), authPayload.length());

	doc.Parse("{ }");
	doc.AddMember("apiKey", apiKeyValJson, doc.GetAllocator());
	doc.AddMember("authSig", sigValJson, doc.GetAllocator());
	doc.AddMember("authNonce", nonceValJson, doc.GetAllocator());
	doc.AddMember("authPayload", authPayloadValJson, doc.GetAllocator());
	doc.AddMember("event", "auth", doc.GetAllocator());
	doc.AddMember("calc", 1, doc.GetAllocator());

	GetJsonString(&doc, &out);

	return out;
}

std::string BfxLibrary::CreateMarginInfoPayload()
{
	Document doc;
	std::string out = "";
	out = string_format(std::string("[0,\"calc\",null,[[\"margin_sym_%s\"]]]"), bfxSymbol.c_str());//we do this because rapid json library gets rid of unneeded []
	
	return out;
}

std::string BfxLibrary::CreateOrderPayload(double * amount,int type)
{
	Document doc;
	std::string out = "";
	std::string stringAmount = string_format(std::string("%lf"), *amount);

	if(type == close_position)
		InitalizeWebSocketPayload(&doc, close_position);
	else
		InitalizeWebSocketPayload(&doc, order);

	doc[3].operator[]("symbol").SetString(bfxSymbol.c_str(), bfxSymbol.length());
	doc[3].operator[]("amount").SetString(stringAmount.c_str(), stringAmount.length());

	GetJsonString(&doc, &out);

	return out;
}

int BfxLibrary::WaitForData(double timeoutMilliseconds)
{
	HighResolutionTimePoint startTime = std::chrono::high_resolution_clock::now();
	HighResolutionTimePoint currTime = startTime;
	int timeout = SUCCESS;
	DoubleMili diff;

	while (webSocketDataRecieved == false)
	{
		if (timeoutMilliseconds > 0)//if 0 or less wait indefinetly
		{
			diff = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - startTime);
			if (diff.count() > timeoutMilliseconds)
			{
				timeout = WebSocketGetDataTimeout;
				break;
			}

		}
		usleep(10000);//sleep for 10 miliseconds
		currTime = std::chrono::high_resolution_clock::now();
	}
	return timeout;
}

int BfxLibrary::SendRecieveWebSocketData(ix::WebSocket* webSocket, std::string* in, std::string* out)
{
	int cc = SUCCESS;

	if (webSocket->getReadyState() == ReadyState::Open)
	{
		webSocketDataRecieved = false;
		webSocket->send(in->c_str());

		cc = WaitForData(5000);
		if (cc == SUCCESS)
		{
			*out = BfxLibrary::webSocketRecievedData;
		}
		else
		{
			LOG_ERROR_CODE(cc);
		}
	}
	else
		cc = WebSocketNotConnected;

	return cc;
}

ReadyState BfxLibrary::GetWebSocketTradeChannelStatus()
{
	return webSocketTradeChannel.getReadyState();
}

ReadyState BfxLibrary::GetWebSocketAuthStatus()
{
	return webSocketAuth.getReadyState();
}

double BfxLibrary::GetCurrentPrice()
{
	return currentPrice;
}