#pragma once
#include "global.h"
#include "BfxBotConfig.h"

#define GET_DAILY_CANDLES "https://api-pub.bitfinex.com/v2/candles/trade:1D:%s/hist?start=%lld&end=%lld"
#define GET_SYMBOL_DETAILS "https://api.bitfinex.com/v1/symbols_details"
#define GET_TIMEFRAME_CANDLES "https://api-pub.bitfinex.com/v2/candles/trade:%s:%s/hist?limit=2"
#define WebSocketURL "wss://api.bitfinex.com/ws/2"

static size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
	data->append((char*)ptr, size * nmemb);
	return size * nmemb;
}

class BfxLibrary
{
	public:
		BfxLibrary();
		~BfxLibrary();
		int ConnectWebSocketTradeChannel();
		int DisconnetWebSocketTradeChannel();
		int ConnectWebSocketAuth();
		int DisconnetWebSocketAuth();
		int SendAuthWebSocket(std::string * symbol);
		void InitKeys(char* apiKey, char* secretKey);
		int  GetWeeklyPivot(double* pivot_point_out, double* s1_short_out, double* s2_short_out, double* s3_short_out, double* r1_long_out, double* r2_long_out, double* r3_long_out, std::string* symbol);
		int  GetTimeFramePivot(ConfigTimeFrame candleTimeFrame, unsigned long long timeFrame, double* pivot_point_out, double* s1_short_out, double* s2_short_out, double* s3_short_out, double* r1_long_out, double* r2_long_out, double* r3_long_out, std::string* symbol);
		int  GetMinimumOrderSize(std::string* symbol,double* minimum_out);
		double GetCurrentPrice();
		int    SubscribeToTradeChannel(std::string* symbol);
		double GetCurrentPositionAmount();
		int    GetTradeableBalance(double* balance_out,double* balance_pl_out);
		int    CreateClosePosition(double* amount);//amount should be minimum
		int    CreateOrder(double* amount);//amount should be less than tradeable balance
		ReadyState GetWebSocketTradeChannelStatus();
		ReadyState GetWebSocketAuthStatus();
	private:
		std::string apiKey;
		std::string secretKey;
		std::string GetSignature(std::string* payload);
		std::string GetBase64(std::string input);
		void InitalizeBasePayload(Document* in, char* request);
		void InitalizeWebSocketPayload(Document* in, int type);
		int ParseResponseData(Document* out);
		void InitCurl();

		void AddAuthHeaders(CURL* curl, struct curl_slist **chunk, std::string* payload, std::string* signature);
		void CreateOutputPayload(CURL* curl, struct curl_slist **in_chunk);
		std::string CreateSubscribePayload(std::string* symbol);
		std::string CreateWebSocketAuthPayload();
		std::string CreateMarginInfoPayload();
		std::string CreateOrderPayload(double* amount, int type);

		void GetBfxWeeklyTimeSpan(long long* startTime_out, long long* endTime_out);
		void CalculateWeeklyPivotData(Document* doc, double* pivot_point_out, double* s1_short_out, double* s2_short_out, double* s3_short_out, double* r1_long_out, double* r2_long_out, double* r3_long_out);
		void CalculateTimeFramePivotData(Document* doc, unsigned long long timeFrame,double* pivot_point_out, double* s1_short_out, double* s2_short_out, double* s3_short_out, double* r1_long_out, double* r2_long_out, double* r3_long_out);
		void FindMinimumOrderSizeSymbol(Document* doc, std::string* symbol, double* minimum_out);
		void ExtractCurrentPrice(std::string* str);
		void ExtractPositionSize(std::string* str);
		void ExtractPositionSizeUpdates(std::string* str);
		void ExtractTradeableBalance(Document* doc,double* balance_out, double* balance_pl_out);


		int SendRecieveWebSocketData(ix::WebSocket* webSocket, std::string* in, std::string* out);
		int WaitForData(double timeoutMilliseconds);
		
		CURL* curl;
		ix::WebSocket webSocketTradeChannel;
		ix::WebSocket webSocketAuth;
		std::string responseData;
		std::string webSocketRecievedData;
		std::string inputData;
		std::string base64Payload;
		std::string signature;
		std::string nonce;
		std::string authStatus;
		std::string bfxSymbol;
		std::string responseString;
		long response_code;
		Value nonceValJson;
		Value requestValJson;
		Value apiKeyValJson;
		Value sigValJson;
		Value authPayloadValJson;
		double currentPrice;
		double currentPositionAmount;
		double tradeableBalance;
		bool webSocketDataRecieved;
		int auth_channel_id;

		static void recieveCallBackTradeChannel(WebSocketMessagePtr socketMessage,
			void* objectPtr);

		static void recieveCallBackAuth(WebSocketMessagePtr socketMessage,
			void* objectPtr);
		
		enum{subscribe_trading,order,close_position};
};
