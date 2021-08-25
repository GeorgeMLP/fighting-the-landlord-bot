#include <iostream>
#include <vector>
#include <iterator>
#include <climits>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <set>
#include "jsoncpp/json.h"
using namespace std;


vector<string> requests[3];
vector<string> responses[3];
vector<short> cards;
set<short> player_initial_cards[3];
set<short> player_cards[3];
set<short> public_cards;
string bots[3] = { "bot0.exe", "bot1.exe", "bot2.exe" };
vector<string> bots_positions[6];  // 3个bot的位置全排列
vector<int> bots_no[6];  // 全排列后的位置编号


constexpr short card2level(short card)
{
	return card / 4 + card / 53;
}


template <class T>
void print_vector(vector<T> vec)
{
	copy(vec.begin(), vec.end(), ostream_iterator<T>(cout, " "));
	cout << endl;
}


template <class T>
void print_set(set<T> set)
{
	for (T c : set)
	{
		int j = card2level(c) + 3;
		if (j == 14)j = 1;
		else if (j == 15)j = 2;
		else if (j == 16)j = 0;
		else if (j == 17)j = -1;
		std::cout << j << " ";
	}
	cout << endl;
}


string get_response(const char* cmd)
{
	char result[2048] = "";
	char buf_ps[2048];
	char ps[2048] = {};
	FILE* ptr;
	strcpy_s(ps, cmd);  // 如编译不通过的话将strcpy_s改成strcpy
	if ((ptr = _popen(ps, "r")) != NULL)
	{
		while (fgets(buf_ps, 2048, ptr) != NULL)strcat_s(result, buf_ps);
		_pclose(ptr);
		ptr = NULL;
	}
	else cout << "_popen " << ps << " error" << endl;
	string ret(result);
	return ret;
}


int main()
{
	srand(time(nullptr));
	int player_winning_times[3] = {};
	int landlord_position;
	for (short card = 0; card < 54; card++)cards.push_back(card);
	bots_positions[0].push_back(bots[0]); bots_positions[0].push_back(bots[1]); bots_positions[0].push_back(bots[2]);
	bots_positions[1].push_back(bots[1]); bots_positions[1].push_back(bots[2]); bots_positions[1].push_back(bots[0]);
	bots_positions[2].push_back(bots[2]); bots_positions[2].push_back(bots[0]); bots_positions[2].push_back(bots[1]);
	bots_positions[3].push_back(bots[0]); bots_positions[3].push_back(bots[2]); bots_positions[3].push_back(bots[1]);
	bots_positions[4].push_back(bots[2]); bots_positions[4].push_back(bots[1]); bots_positions[4].push_back(bots[0]);
	bots_positions[5].push_back(bots[1]); bots_positions[5].push_back(bots[0]); bots_positions[5].push_back(bots[2]);
	bots_no[0].push_back(0); bots_no[0].push_back(1); bots_no[0].push_back(2);
	bots_no[1].push_back(1); bots_no[1].push_back(2); bots_no[1].push_back(0);
	bots_no[2].push_back(2); bots_no[2].push_back(0); bots_no[2].push_back(1);
	bots_no[3].push_back(0); bots_no[3].push_back(2); bots_no[3].push_back(1);
	bots_no[4].push_back(2); bots_no[4].push_back(1); bots_no[4].push_back(0);
	bots_no[5].push_back(1); bots_no[5].push_back(0); bots_no[5].push_back(2);
	for (int landlord_threshold = -50; landlord_threshold < 20; landlord_threshold++)
	{
		player_winning_times[0] = player_winning_times[1] = player_winning_times[2] = 0;
		int total_games = 50;  // total_games乘以6即为对局总数（因为要全排列打循环赛）
		for (int game_no = 0; game_no < total_games; game_no++)  // 每次循环代表6局对局
		{
#pragma region 准备数据
			srand(rand());
			random_shuffle(cards.begin(), cards.end());
			for (int i = 0; i < 3; i++)player_initial_cards[i].clear();
			public_cards.clear();
			player_initial_cards[0].insert(cards.begin(), cards.begin() + 17);
			player_initial_cards[1].insert(cards.begin() + 17, cards.begin() + 34);
			player_initial_cards[2].insert(cards.begin() + 34, cards.begin() + 51);
			public_cards.insert(cards.begin() + 51, cards.begin() + 54);
			/*cout << "player 0: ";
			print_set(player_initial_cards[0]);
			cout << "player 1: ";
			print_set(player_initial_cards[1]);
			cout << "player 2: ";
			print_set(player_initial_cards[2]);
			cout << "public cards: ";
			print_set(public_cards);*/
			int score = 1;
#pragma endregion
			for (int sub_game = 0; sub_game < 3; sub_game++)
			{
#pragma region 叫分阶段
				for (int i = 0; i < 3; i++)requests[i].clear();
				for (int i = 0; i < 3; i++)responses[i].clear();
				for (int i = 0; i < 3; i++)player_cards[i] = player_initial_cards[i];
				int player_bid[3];
				int final_bid;
				string requests0, responses0, cmd0;
				requests0 += "{\"\"own\"\":[";
				requests0 += to_string(*player_cards[0].begin());
				for (set<short>::iterator i = next(player_cards[0].begin()); i != player_cards[0].end(); i++)
				{
					requests0 += ",";
					requests0 += to_string(*i);
				}
				requests0 += "],\"\"bid\"\":[]}";
				responses0 = "";
				requests[0].push_back(requests0);
				cmd0 = bots_positions[sub_game][0] + " \"{\"\"requests\"\":[" + requests0 + "],\"\"responses\"\":[" + responses0 + "]}\"";
				// <调参>
				if (bots_no[sub_game][0] == 2)
				{
					cmd0 += " ";
					cmd0 += to_string(landlord_threshold);
				}
				// </调参>
				//cout << "cmd0: " << cmd0 << endl;
				string response0 = get_response(cmd0.c_str());
				Json::Reader reader;
				Json::Value input0;
				reader.parse(response0, input0);
				player_bid[0] = input0["response"].asInt();
				responses[0].push_back(to_string(player_bid[0]));
				//cout << "player_bid[" << bots_no[sub_game][0] << "]: " << player_bid[0] << endl;
				string requests1, responses1, cmd1;
				requests1 += "{\"\"own\"\":[";
				requests1 += to_string(*player_cards[1].begin());
				for (set<short>::iterator i = next(player_cards[1].begin()); i != player_cards[1].end(); i++)
				{
					requests1 += ",";
					requests1 += to_string(*i);
				}
				requests1 += "],\"\"bid\"\":[";
				requests1 += to_string(player_bid[0]);
				requests1 += "]}";
				responses1 = "";
				requests[1].push_back(requests1);
				cmd1 = bots_positions[sub_game][1] + " \"{\"\"requests\"\":[" + requests1 + "],\"\"responses\"\":[" + responses1 + "]}\"";
				// <调参>
				if (bots_no[sub_game][1] == 2)
				{
					cmd1 += " ";
					cmd1 += to_string(landlord_threshold);
				}
				// </调参>
				//cout << "cmd1: " << cmd1 << endl;
				string response1 = get_response(cmd1.c_str());
				Json::Value input1;
				reader.parse(response1, input1);
				player_bid[1] = input1["response"].asInt();
				responses[1].push_back(to_string(player_bid[1]));
				//cout << "player_bid[" << bots_no[sub_game][1] << "]: " << player_bid[1] << endl;
				string requests2, responses2, cmd2;
				requests2 += "{\"\"own\"\":[";
				requests2 += to_string(*player_cards[2].begin());
				for (set<short>::iterator i = next(player_cards[2].begin()); i != player_cards[2].end(); i++)
				{
					requests2 += ",";
					requests2 += to_string(*i);
				}
				requests2 += "],\"\"bid\"\":[";
				requests2 += to_string(player_bid[0]);
				requests2 += ",";
				requests2 += to_string(player_bid[1]);
				requests2 += "]}";
				responses2 = "";
				requests[2].push_back(requests2);
				cmd2 = bots_positions[sub_game][2] + " \"{\"\"requests\"\":[" + requests2 + "],\"\"responses\"\":[" + responses2 + "]}\"";
				// <调参>
				if (bots_no[sub_game][2] == 2)
				{
					cmd2 += " ";
					cmd2 += to_string(landlord_threshold);
				}
				// </调参>
				//cout << "cmd2: " << cmd2 << endl;
				string response2 = get_response(cmd2.c_str());
				Json::Value input2;
				reader.parse(response2, input2);
				player_bid[2] = input2["response"].asInt();
				responses[2].push_back(to_string(player_bid[2]));
				//cout << "player_bid[" << bots_no[sub_game][2] << "]: " << player_bid[2] << endl;
				landlord_position = 0;
				if (player_bid[1] > player_bid[0])
				{
					if (player_bid[2] > player_bid[1])landlord_position = 2;
					else landlord_position = 1;
				}
				else if (player_bid[2] > player_bid[0])landlord_position = 2;
				final_bid = *max_element(player_bid, player_bid + 3);
				//cout << "final_bid: " << final_bid << endl;
				score = max(final_bid, 1);
#pragma endregion
#pragma region 出牌阶段
				int turn = landlord_position;
				set<short> history[2];  // history[0]为上上家的出牌记录，history[1]为上家的出牌记录
				// 先进行第一回合
				for (int i = 0; i < 3; i++)
				{
					string tmp_requests, tmp_responses, tmp_cmd, tmp_response;
					tmp_requests += "{\"\"history\"\":[[";
					if (history[0].size())
					{
						tmp_requests += to_string(*history[0].begin());
						for (set<short>::iterator j = next(history[0].begin()); j != history[0].end(); j++)
						{
							tmp_requests += ",";
							tmp_requests += to_string(*j);
						}
					}
					tmp_requests += "],[";
					if (history[1].size())
					{
						tmp_requests += to_string(*history[1].begin());
						for (set<short>::iterator j = next(history[1].begin()); j != history[1].end(); j++)
						{
							tmp_requests += ",";
							tmp_requests += to_string(*j);
						}
					}
					tmp_requests += "]],\"\"own\"\":[";
					tmp_requests += to_string(*player_cards[turn].begin());
					for (set<short>::iterator j = next(player_cards[turn].begin()); j != player_cards[turn].end(); j++)
					{
						tmp_requests += ",";
						tmp_requests += to_string(*j);
					}
					tmp_requests += "],\"\"publiccard\"\":[";
					tmp_requests += to_string(*public_cards.begin());
					for (set<short>::iterator j = next(public_cards.begin()); j != public_cards.end(); j++)
					{
						tmp_requests += ",";
						tmp_requests += to_string(*j);
					}
					tmp_requests += "],\"\"landlord\"\":";
					tmp_requests += to_string(landlord_position);
					tmp_requests += ",\"\"pos\"\":";
					tmp_requests += to_string(turn);
					tmp_requests += ",\"\"finalbid\"\":";
					tmp_requests += to_string(final_bid);
					tmp_requests += "}";
					requests[turn].push_back(tmp_requests);
					tmp_cmd += bots_positions[sub_game][turn];
					tmp_cmd += " \"{\"\"requests\"\":[";
					tmp_cmd += requests[turn][0];
					tmp_cmd += ",";
					tmp_cmd += requests[turn][1];
					tmp_cmd += "],\"\"responses\"\":[";
					tmp_cmd += *responses[turn].begin();
					tmp_cmd += "]}\"";
					// <调参>
					if (bots_no[sub_game][turn] == 2)
					{
						tmp_cmd += " ";
						tmp_cmd += to_string(landlord_threshold);
					}
					// </调参>
					//cout << "tmp_cmd: " << tmp_cmd << endl;
					tmp_response = get_response(tmp_cmd.c_str());
					Json::Value output;
					Json::Reader reader;
					reader.parse(tmp_response, output);
					history[0] = history[1];
					history[1].clear();
					for (int j = 0; j < output["response"].size(); j++)history[1].insert(output["response"][j].asInt());
					if (i == 0)player_cards[landlord_position].insert(public_cards.begin(), public_cards.end());
					/*cout << bots_no[sub_game][turn] << ": ";
					print_set(history[1]);*/
					for (short card : history[1])player_cards[turn].erase(card);
					if (history[1].size() == 2 && history[1].find(52) != history[1].end() && history[1].find(53) != history[1].end())score *= 2;  // 王炸
					else if (history[1].size() == 4 && *history[1].begin() == *prev(history[1].end()))score *= 2;  // 炸弹
					tmp_responses.clear();
					tmp_responses += "[";
					if (history[1].size())
					{
						tmp_responses += to_string(*history[1].begin());
						for (set<short>::iterator j = next(history[1].begin()); j != history[1].end(); j++)
						{
							tmp_responses += ",";
							tmp_responses += to_string(*j);
						}
					}
					tmp_responses += "]";
					responses[turn].push_back(tmp_responses);
					turn = (turn + 1) % 3;
				}
				// 再进行接下来的回合
				bool landlord_has_not_played = true;
				while (true)
				{
					string tmp_requests, tmp_responses, tmp_cmd, tmp_response;
					tmp_requests += "{\"\"history\"\":[[";
					if (history[0].size())
					{
						tmp_requests += to_string(*history[0].begin());
						for (set<short>::iterator j = next(history[0].begin()); j != history[0].end(); j++)
						{
							tmp_requests += ",";
							tmp_requests += to_string(*j);
						}
					}
					tmp_requests += "],[";
					if (history[1].size())
					{
						tmp_requests += to_string(*history[1].begin());
						for (set<short>::iterator j = next(history[1].begin()); j != history[1].end(); j++)
						{
							tmp_requests += ",";
							tmp_requests += to_string(*j);
						}
					}
					tmp_requests += "]]}";
					requests[turn].push_back(tmp_requests);
					tmp_cmd += bots_positions[sub_game][turn];
					tmp_cmd += " \"{\"\"requests\"\":[";
					tmp_cmd += *requests[turn].begin();
					for (vector<string>::iterator i = next(requests[turn].begin()); i != requests[turn].end(); i++)
					{
						tmp_cmd += ",";
						tmp_cmd += *i;
					}
					tmp_cmd += "],\"\"responses\"\":[";
					tmp_cmd += *responses[turn].begin();
					for (vector<string>::iterator i = next(responses[turn].begin()); i != responses[turn].end(); i++)
					{
						tmp_cmd += ",";
						tmp_cmd += *i;
					}
					tmp_cmd += "]}\"";
					// <调参>
					if (bots_no[sub_game][turn] == 2)
					{
						tmp_cmd += " ";
						tmp_cmd += to_string(landlord_threshold);
					}
					// </调参>
					//cout << "tmp_cmd: " << tmp_cmd << endl;
					tmp_response = get_response(tmp_cmd.c_str());
					Json::Value output;
					Json::Reader reader;
					reader.parse(tmp_response, output);
					history[0] = history[1];
					history[1].clear();
					for (int i = 0; i < output["response"].size(); i++)history[1].insert(output["response"][i].asInt());
					/*cout << bots_no[sub_game][turn] << ": ";
					print_set(history[1]);*/
					for (short card : history[1])player_cards[turn].erase(card);
					if (history[1].size() == 2 && history[1].find(52) != history[1].end() && history[1].find(53) != history[1].end())score *= 2;  // 王炸
					else if (history[1].size() == 4 && *history[1].begin() == *prev(history[1].end()))score *= 2;  // 炸弹
					if (turn == landlord_position && history[1].size())landlord_has_not_played = false;  // 地主除了第一手之外出过牌了
					tmp_responses.clear();
					tmp_responses += "[";
					if (history[1].size())
					{
						tmp_responses += to_string(*history[1].begin());
						for (set<short>::iterator j = next(history[1].begin()); j != history[1].end(); j++)
						{
							tmp_responses += ",";
							tmp_responses += to_string(*j);
						}
					}
					tmp_responses += "]";
					responses[turn].push_back(tmp_responses);
					if (!player_cards[turn].size())
					{
						if (turn == landlord_position)  // 地主胜利
						{
							if (player_cards[(turn + 1) % 3].size() == 17 && player_cards[(turn + 2) % 3].size() == 17)score *= 2;  // 地主春天
							player_winning_times[bots_no[sub_game][turn]] += 2 * score;
							for (int i = 0; i < 3; i++)
								if (i != turn)player_winning_times[bots_no[sub_game][i]] -= score;
						}
						else  // 农民胜利
						{
							if (landlord_has_not_played)score *= 2;  // 农民春天
							player_winning_times[bots_no[sub_game][landlord_position]] -= 2 * score;
							for (int i = 0; i < 3; i++)
								if (i != landlord_position)player_winning_times[bots_no[sub_game][i]] += score;
						}
						/*cout << "over, winner: " << bots_no[sub_game][turn];
						if (turn != landlord_position)cout << ", " << bots_no[sub_game][3 - turn - landlord_position];
						cout << "; landlord: " << bots_no[sub_game][landlord_position];
						cout << "; score: " << score << endl;*/
						break;
					}
					turn = (turn + 1) % 3;
				}
#pragma endregion
			}
		}
		cout << "landlord threshold = " << landlord_threshold << endl;
		cout << "result: " << endl;
		cout << "bot0: " << player_winning_times[0] << endl;
		cout << "bot1: " << player_winning_times[1] << endl;
		cout << "bot2: " << player_winning_times[2] << endl;
	}
}