#include "BfxTradingBotMain.h"

void sig_term_handler(int signum)
{
	if (signum == SIGTERM || signum == SIGQUIT || signum == SIGKILL)
		TerminateApplication = true;
}


int main(int argc, char** argv)
{
	int cc = SUCCESS;
	std::string configPath = "";

	if(argc > 1)
		configPath = argv[1];
	else
	{
		cc = ErrorNumParams;
		LOG_ERROR_CODE(cc);
	}
	BfxTradingBotMain bfxTradingBotMain;


	struct sigaction new_action, old_action;

	new_action.sa_handler = sig_term_handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;


	sigaction(SIGTERM, &new_action, NULL);

	sigaction(SIGQUIT, &new_action, NULL);

	sigaction(SIGKILL, &new_action, NULL);

	cc = curl_global_init(CURL_GLOBAL_ALL);
	
	if (cc == SUCCESS)
	{
		cc = bfxTradingBotMain.Init((char*)configPath.c_str());

		if (cc == SUCCESS)
		{
			while (TerminateApplication == false)
			{
				cc = bfxTradingBotMain.Process();

				usleep(10000);//sleep for 10 milliseconds
			}
		}
	}
	
	LOG_INFO("Shutting Down...");
	bfxTradingBotMain.End();//end trade processing thread

	if (cc != SUCCESS)
	{
		bfxTradingBotMain.SendCurrentErrorFile();
	}
	
	curl_global_cleanup();

    return cc;
}