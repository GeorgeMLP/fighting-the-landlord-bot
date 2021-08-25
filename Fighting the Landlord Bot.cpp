#include <iostream>
#include <set>
#include <string>
#include <cassert>
#include <cstring> // 注意memset是cstring里的
#include <algorithm>
#include "jsoncpp/json.h" // 在平台上，C++编译时默认包含此库
#include <functional>
#include <iterator>
#include <ctime>
#include <map>
#include <utility>
#include <cmath>
#include <climits>

using std::set;
using std::sort;
using std::string;
using std::unique;
using std::vector;

constexpr int PLAYER_COUNT = 3;

#pragma region 基本定义
enum class Stage
{
	BIDDING, // 叫分阶段
	PLAYING	 // 打牌阶段
};

enum class CardComboType
{
	PASS,		// 过
	SINGLE,		// 单张
	PAIR,		// 对子
	STRAIGHT,	// 顺子
	STRAIGHT2,	// 双顺
	TRIPLET,	// 三条
	TRIPLET1,	// 三带一
	TRIPLET2,	// 三带二
	BOMB,		// 炸弹
	QUADRUPLE2, // 四带二（只）
	QUADRUPLE4, // 四带二（对）
	PLANE,		// 飞机
	PLANE1,		// 飞机带小翼
	PLANE2,		// 飞机带大翼
	SSHUTTLE,	// 航天飞机
	SSHUTTLE2,	// 航天飞机带小翼
	SSHUTTLE4,	// 航天飞机带大翼
	ROCKET,		// 火箭
	INVALID		// 非法牌型
};

int cardComboScores[] = {
	0,	// 过
	1,	// 单张
	2,	// 对子
	6,	// 顺子
	6,	// 双顺
	4,	// 三条
	4,	// 三带一
	4,	// 三带二
	10, // 炸弹
	8,	// 四带二（只）
	8,	// 四带二（对）
	8,	// 飞机
	8,	// 飞机带小翼
	8,	// 飞机带大翼
	10, // 航天飞机（需要特判：二连为10分，多连为20分）
	10, // 航天飞机带小翼
	10, // 航天飞机带大翼
	16, // 火箭
	0	// 非法牌型
};

#ifndef _BOTZONE_ONLINE
string cardComboStrings[] = {
	"PASS",
	"SINGLE",
	"PAIR",
	"STRAIGHT",
	"STRAIGHT2",
	"TRIPLET",
	"TRIPLET1",
	"TRIPLET2",
	"BOMB",
	"QUADRUPLE2",
	"QUADRUPLE4",
	"PLANE",
	"PLANE1",
	"PLANE2",
	"SSHUTTLE",
	"SSHUTTLE2",
	"SSHUTTLE4",
	"ROCKET",
	"INVALID" };
#endif

// 用0~53这54个整数表示唯一的一张牌
using Card = short;
constexpr Card card_joker = 52;
constexpr Card card_JOKER = 53;

// 除了用0~53这54个整数表示唯一的牌，
// 这里还用另一种序号表示牌的大小（不管花色），以便比较，称作等级（Level）
// 对应关系如下：
// 3 4 5 6 7 8 9 10	J Q K	A	2	小王	大王
// 0 1 2 3 4 5 6 7	8 9 10	11	12	13	14
using Level = short;
constexpr Level MAX_LEVEL = 15;
constexpr Level MAX_STRAIGHT_LEVEL = 11;
constexpr Level level_joker = 13;
constexpr Level level_JOKER = 14;

/**
* 将Card（取值范围0~53）变成Level（取值范围0~14）
*/
constexpr Level card2level(Card card)
{
	return card / 4 + card / 53;
}
#pragma endregion

#pragma region 用来表示若干张牌的结构体，有很多有用的成员函数，见下面的注释
/**
* 本结构体用来表示若干张扑克牌的组合
* 成员结构体CardPack用来记录每种牌出现的次数，每个CardPack代表一种牌；对vector<CardPack>进行排序时，按照牌的数量从大到小排序，若相等则按其Level排序
* 成员函数findMaxSeq计算数量最多的CardPack从头开始递减了几个；例如，888877775555999444663这些牌，从数量最多的8888开始，递减到7777，而5555与7777不连续，故函数返回2；
*	再比如，999888777554463这些牌，从数量最多的999开始，递减到888，再递减到777，而55与777数量不同，故函数返回3）
* 成员函数getWeight返回本结构体代表的这些牌的权重（各种牌型的权重见基本定义）
* 第一个构造函数创建一个空的CardCombo，牌型为PASS，没什么好说的（注意牌型通过枚举类comboType记录）
* 第二个构造函数将[begin,end)区间内的牌作为一个CardCombo，同时判断出这些牌是什么牌型（如单张、顺子、三带一、不合法，等等），用comboType记录
* 第三个构造函数与第二个类似，但多输入了一个_comboType代表[begin,end)中的牌的牌型，这样在构造函数中就不用判断牌型了，效率更高
* 成员函数canBeBeatenBy输入一个CardCombo，判断自己的牌能否大过输入的牌
* 成员函数findFirstValid在区间[begin,end)中找出比当前牌组（即本结构体所记录的牌组）更大的牌组，例如当前牌组为3334，那我们希望在[begin,end)中找到更大的牌组，如5553、6668等；
*	这个函数的逻辑是先找主牌再找从牌，并且递增地找（例如当前牌组的主牌是333，那先找444，再找555，以此类推），只要数量够了，就选中，也不管这样出牌会不会拆顺子或炸弹
*	（例如，当前牌组是3，[begin,end)中的牌是44445，那这个函数就直接返回4，把炸弹拆掉），因此还有很大提升空间
* 成员函数debugPrint输出本牌组的相关信息，可用于debug
* 成员函数printCards将cards排序后输出cards中的所有牌
*/
struct CardCombo
{
	// 表示同等级的牌有多少张
	// 会按个数从大到小、等级从大到小排序
	struct CardPack
	{
		Level level;
		short count;

		bool operator<(const CardPack& b) const
		{
			if (count == b.count)
				return level > b.level;
			return count > b.count;
		}

		bool operator ==(const CardPack& b) const
		{
			return level == b.level && count == b.count;
		}
	};
	vector<Card> cards;		 // 原始的牌，未排序
	vector<CardPack> packs;	 // 按数目和大小排序的牌种（CardPack结构体代表一种牌，同时记录了这种牌的数量）
	CardComboType comboType; // 算出的牌型
	Level comboLevel = 0;	 // 算出的大小序（最多的那种牌的Level）

	/**
	* 检查个数最多的CardPack递减了几个
	*/
	int findMaxSeq() const
	{
		for (unsigned c = 1; c < packs.size(); c++)
			if (packs[c].count != packs[0].count ||
				packs[c].level != packs[c - 1].level - 1)
				return c;
		return packs.size();
	}

	/**
	* 这个牌型最后算总分的时候的权重
	*/
	int getWeight() const
	{
		if (comboType == CardComboType::SSHUTTLE ||
			comboType == CardComboType::SSHUTTLE2 ||
			comboType == CardComboType::SSHUTTLE4)
			return cardComboScores[(int)comboType] + (findMaxSeq() > 2) * 10;
		return cardComboScores[(int)comboType];
	}

	// 创建一个空牌组
	CardCombo() : comboType(CardComboType::PASS) {}

	/**
	* 通过Card（即short）类型的迭代器创建一个牌型
	* 并计算出牌型和大小序等
	* 假设输入没有重复数字（即重复的Card）
	*/
	template <typename CARD_ITERATOR>
	CardCombo(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		// 特判：空
		if (begin == end)
		{
			comboType = CardComboType::PASS;
			return;
		}

		// 每种牌有多少个
		short counts[MAX_LEVEL + 1] = {};

		// 同种牌的张数（有多少个单张、对子、三条、四条）
		short countOfCount[5] = {};

		cards = vector<Card>(begin, end);
		for (Card c : cards)
			counts[card2level(c)]++;
		for (Level l = 0; l <= MAX_LEVEL; l++)
			if (counts[l])
			{
				packs.push_back(CardPack{ l, counts[l] });
				countOfCount[counts[l]]++;
			}
		sort(packs.begin(), packs.end());

		// 用最多的那种牌总是可以比较大小的
		comboLevel = packs[0].level;

		// 计算牌型
		// 按照 同种牌的张数 有几种 进行分类
		vector<int> kindOfCountOfCount;
		for (int i = 0; i <= 4; i++)
			if (countOfCount[i])
				kindOfCountOfCount.push_back(i);
		sort(kindOfCountOfCount.begin(), kindOfCountOfCount.end());

		int curr, lesser;

		switch (kindOfCountOfCount.size())
		{
		case 1: // 只有一类牌
			curr = countOfCount[kindOfCountOfCount[0]];
			switch (kindOfCountOfCount[0])
			{
			case 1:
				// 只有若干单张
				if (curr == 1)
				{
					comboType = CardComboType::SINGLE;
					return;
				}
				if (curr == 2 && packs[1].level == level_joker)
				{
					comboType = CardComboType::ROCKET;
					return;
				}
				if (curr >= 5 && findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::STRAIGHT;
					return;
				}
				break;
			case 2:
				// 只有若干对子
				if (curr == 1)
				{
					comboType = CardComboType::PAIR;
					return;
				}
				if (curr >= 3 && findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::STRAIGHT2;
					return;
				}
				break;
			case 3:
				// 只有若干三条
				if (curr == 1)
				{
					comboType = CardComboType::TRIPLET;
					return;
				}
				if (findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::PLANE;
					return;
				}
				break;
			case 4:
				// 只有若干四条
				if (curr == 1)
				{
					comboType = CardComboType::BOMB;
					return;
				}
				if (findMaxSeq() == curr &&
					packs.begin()->level <= MAX_STRAIGHT_LEVEL)
				{
					comboType = CardComboType::SSHUTTLE;
					return;
				}
			}
			break;
		case 2: // 有两类牌
			curr = countOfCount[kindOfCountOfCount[1]];
			lesser = countOfCount[kindOfCountOfCount[0]];
			if (kindOfCountOfCount[1] == 3)
			{
				// 三条带？
				if (kindOfCountOfCount[0] == 1)
				{
					// 三带一
					if (curr == 1 && lesser == 1)
					{
						comboType = CardComboType::TRIPLET1;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::PLANE1;
						return;
					}
				}
				if (kindOfCountOfCount[0] == 2)
				{
					// 三带二
					if (curr == 1 && lesser == 1)
					{
						comboType = CardComboType::TRIPLET2;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::PLANE2;
						return;
					}
				}
			}
			if (kindOfCountOfCount[1] == 4)
			{
				// 四条带？
				if (kindOfCountOfCount[0] == 1)
				{
					// 四条带两只 * n
					if (curr == 1 && lesser == 2)
					{
						comboType = CardComboType::QUADRUPLE2;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr * 2 &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::SSHUTTLE2;
						return;
					}
				}
				if (kindOfCountOfCount[0] == 2)
				{
					// 四条带两对 * n
					if (curr == 1 && lesser == 2)
					{
						comboType = CardComboType::QUADRUPLE4;
						return;
					}
					if (findMaxSeq() == curr && lesser == curr * 2 &&
						packs.begin()->level <= MAX_STRAIGHT_LEVEL)
					{
						comboType = CardComboType::SSHUTTLE4;
						return;
					}
				}
			}
		}

		comboType = CardComboType::INVALID;
	}

	/**
	* 和上一个构造函数类似，但多输入了应该_comboType
	* 这样在构造函数中就不用再去判断牌型了，效率更高
	* 使用该构造函数需保证所提供的_comboType是正确的（即与[begin,end)中的牌的牌型一致）
	*/
	template <typename CARD_ITERATOR>
	CardCombo(CARD_ITERATOR begin, CARD_ITERATOR end, CardComboType _comboType) :comboType(_comboType)
	{
		if (_comboType == CardComboType::PASS)return;
		short counts[MAX_LEVEL + 1] = {};
		cards = vector<Card>(begin, end);
		for (Card c : cards)counts[card2level(c)]++;
		for (Level l = 0; l <= MAX_LEVEL; l++)
			if (counts[l])
				packs.push_back(CardPack{ l, counts[l] });
		sort(packs.begin(), packs.end());
		comboLevel = packs[0].level;
		// 后面就不用判断comboType了
	}

	/**
	* 判断指定牌组能否大过当前牌组（这个函数不考虑过牌的情况！）
	*/
	bool canBeBeatenBy(const CardCombo& b) const
	{
		if (comboType == CardComboType::INVALID || b.comboType == CardComboType::INVALID)
			return false;
		if (b.comboType == CardComboType::ROCKET)
			return true;
		if (b.comboType == CardComboType::BOMB)
			switch (comboType)
			{
			case CardComboType::ROCKET:
				return false;
			case CardComboType::BOMB:
				return b.comboLevel > comboLevel;
			default:
				return true;
			}
		return b.comboType == comboType && b.cards.size() == cards.size() && b.comboLevel > comboLevel;
	}

	/**
	* 从指定手牌中寻找第一个能大过当前牌组的牌组
	* 如果随便出的话只出第一张
	* 如果不存在则返回一个PASS的牌组
	*/
	template <typename CARD_ITERATOR>
	CardCombo findFirstValid(CARD_ITERATOR begin, CARD_ITERATOR end) const
	{
		if (comboType == CardComboType::PASS) // 如果不需要大过谁，只需要随便出
		{
			CARD_ITERATOR second = begin;
			second++;
			return CardCombo(begin, second); // 那么就出第一张牌……
		}

		// 然后先看一下是不是火箭，是的话就过
		if (comboType == CardComboType::ROCKET)
			return CardCombo();

		// 现在打算从手牌中凑出同牌型的牌
		auto deck = vector<Card>(begin, end); // 手牌
		short counts[MAX_LEVEL + 1] = {};

		unsigned short kindCount = 0;

		// 先数一下手牌里每种牌有多少个
		for (Card c : deck)
			counts[card2level(c)]++;

		// 手牌如果不够用，直接不用凑了，看看能不能炸吧
		if (deck.size() < cards.size())
			goto failure;

		// 再数一下手牌里有多少种牌
		for (short c : counts)
			if (c)
				kindCount++;

		// 否则不断增大当前牌组的主牌，看看能不能找到匹配的牌组
		{
			// 开始增大主牌
			int mainPackCount = findMaxSeq();
			bool isSequential =
				comboType == CardComboType::STRAIGHT ||
				comboType == CardComboType::STRAIGHT2 ||
				comboType == CardComboType::PLANE ||
				comboType == CardComboType::PLANE1 ||
				comboType == CardComboType::PLANE2 ||
				comboType == CardComboType::SSHUTTLE ||
				comboType == CardComboType::SSHUTTLE2 ||
				comboType == CardComboType::SSHUTTLE4;
			for (Level i = 1;; i++) // 增大多少
			{
				for (int j = 0; j < mainPackCount; j++) // 找更大的主牌
				{
					int level = packs[j].level + i;

					// 各种连续牌型的主牌不能到2，非连续牌型的主牌不能到小王，单张的主牌不能超过大王
					if ((comboType == CardComboType::SINGLE && level > MAX_LEVEL) ||
						(isSequential && level > MAX_STRAIGHT_LEVEL) ||
						(comboType != CardComboType::SINGLE && !isSequential && level >= level_joker))
						goto failure;

					// 如果手牌中这种牌不够，就不用继续增了
					if (counts[level] < packs[j].count)
						goto next;
				}
				// 至此我们在手牌[begin,end)中找到了更大的主牌，举个例子，若当前牌组的主牌为333444，或许我们找到了444555或555666等；
				// 接下来我们要找从牌，举个例子，若当前牌组是3334，而我们已经找到了888，那我们只需再找一个从牌，凑成8883、8884之类

				{
					// 找到了合适的主牌，那么从牌呢？
					// 如果手牌的种类数不够，那从牌的种类数就不够，也不行
					if (kindCount < packs.size())
						continue;

					// 好终于可以了
					// 计算每种牌的要求数目吧
					short requiredCounts[MAX_LEVEL + 1] = {};
					for (int j = 0; j < mainPackCount; j++)
						requiredCounts[packs[j].level + i] = packs[j].count;
					// 例如，当前牌组的主牌是333444，我们找到的主牌是555666，那么requiredCounts[2]=3，requiredCounts[3]=3，这里2是5的Level、3是6的Level（见基本定义）
					for (unsigned j = mainPackCount; j < packs.size(); j++)
					{
						Level k;
						for (k = 0; k <= MAX_LEVEL; k++)
						{
							if (requiredCounts[k] || counts[k] < packs[j].count) // 这里的packs[j]代表的是一种从牌（从牌可能有多种，例如四带二，就有两种从牌）
								continue;
							requiredCounts[k] = packs[j].count; // 找到了packs[j]对应的从牌
							break;
						}
						if (k == MAX_LEVEL + 1) // 如果是都不符合要求……就不行了（找不到从牌）
							goto next;
					}

					// 开始产生解
					vector<Card> solve;
					for (Card c : deck)
					{
						Level level = card2level(c);
						if (requiredCounts[level])
						{
							solve.push_back(c);
							requiredCounts[level]--;
						}
					}
					return CardCombo(solve.begin(), solve.end());
				}

			next:; // 再增大
			}
		}

	failure:
		// 实在找不到啊
		// 最后看一下能不能炸吧

		for (Level i = 0; i < level_joker; i++)
			if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // 如果对方是炸弹，能炸的过才行
			{
				// 还真可以啊……
				Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
				return CardCombo(bomb, bomb + 4);
			}

		// 有没有火箭？
		if (counts[level_joker] + counts[level_JOKER] == 2)
		{
			Card rocket[] = { card_joker, card_JOKER };
			return CardCombo(rocket, rocket + 2);
		}

		// ……
		return CardCombo();
	}

	/**
	* 定义CardCombo排序规则：依次对每个CardPack进行排序
	*/
	bool operator <(const CardCombo& _right) const
	{
		int n = std::min(packs.size(), _right.packs.size());
		for (int i = 0; i < n; i++)
		{
			if (packs[i] < _right.packs[i])return true;
			if (_right.packs[i] < packs[i])return false;
		}
		return packs.size() < _right.packs.size();
	}

	bool operator ==(const CardCombo& _right) const
	{
		if (packs.size() != _right.packs.size())return false;
		for (int i = 0; i < packs.size(); i++)
			if (!(packs[i] == _right.packs[i]))
				return false;
		return true;
	}

	void debugPrint()
	{
#ifndef _BOTZONE_ONLINE
		std::cout << "【" << cardComboStrings[(int)comboType] << "共" << cards.size() << "张，大小序" << comboLevel << "】";
#endif
	}

	/**
	* 输出当前牌组的所有牌，0代表小王，-1代表大王
	* 会将cards排序
	*/
	void printCards()
	{
#ifndef _BOTZONE_ONLINE
		sort(cards.begin(), cards.end());
		for (int i : cards)
		{
			int j = card2level(i) + 3;
			if (j == 14)j = 1;
			else if (j == 15)j = 2;
			else if (j == 16)j = 0;
			else if (j == 17)j = -1;
			std::cout << j << " ";
		}
		std::cout << std::endl;
#endif
	}
};
#pragma endregion

#pragma region 用来表示一副手牌的结构体，可以记录下其中所有合法的牌型
/**
* 本结构体用来表示一副手牌，同时可以记录下手牌中的各种合法牌型
* 成员结构体CardPack用来表示一种牌（如3、2、小王等），同时记录这种牌有多少张以及它们的原始编号
* 构造函数将[begin,end)中的所有牌记录在packs中
* 成员函数printCards将这副手牌输出，0代表小王，-1代表大王
* 成员函数getAllSingle~getAllRocket用于统计这副牌中所有合法的某种牌型，并记录在相应的vector<CardCombo>中
*	例如，3344556789这副牌，调用getAllStraight2()，则双顺334455以一个CardCombo结构体的形式被记录在straight2中
* 成员函数getAllCombos将getAllSingle~getAllRocket全部执行一遍，记录手牌中所有合法牌型
* 成员函数printAllCombos先执行getAllCombos()，再将手牌中所有合法牌型输出
* 成员函数erase将[begin,end)中所有牌移除手牌，需保证手牌中有这些牌
* 成员函数insert将[begin,end)中所有牌加入手牌，需保证手牌中没有这些牌
* 成员函数getLength返回手牌中共有多少张牌
*/
struct CardCombinations
{
	// 表示同等级的牌有多少张
	struct CardPack
	{
		Level level;
		short count = 0;
		Card card[4];
	};
	CardPack packs[MAX_LEVEL];	 // 代表一种牌，同时记录了这种牌的数量和牌的原始编号

	// 注意：下面的这些vector记录的是牌的Level，不是牌的原始编号
	vector<CardCombo> single;
	vector<CardCombo> pair;
	vector<CardCombo> straight[13];	//straight[i]表示含i张牌的顺子，下面同理
	vector<CardCombo> straight2[11];
	vector<CardCombo> triplet;
	vector<CardCombo> triplet1;
	vector<CardCombo> triplet2;
	vector<CardCombo> bomb;
	vector<CardCombo> quadruple2;
	vector<CardCombo> quadruple4;
	vector<CardCombo> plane[7];
	vector<CardCombo> plane1[7];
	vector<CardCombo> plane2[7];
	vector<CardCombo> sshuttle;
	vector<CardCombo> sshuttle2;
	vector<CardCombo> sshuttle4;
	vector<CardCombo> rocket;

	/**
	* 默认构造函数
	*/
	CardCombinations() {}

	/**
	* 构造函数，仅仅是将[begin,end)中的所有牌记录在packs中
	*/
	template <typename CARD_ITERATOR>
	CardCombinations(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		for (int i = 0; i < MAX_LEVEL; i++)packs[i].level = i;
		for (CARD_ITERATOR i = begin; i != end; i++)
		{
			Level l = card2level(*i);
			packs[l].card[packs[l].count] = *i;
			packs[l].count++;
		}
	}

	/**
	* 输出当前手牌的所有牌，0代表小王，-1代表大王
	*/
	void printCards()
	{
#ifndef _BOTZONE_ONLINE
		for (int i = 0; i <= 14; i++)
		{
			if (!packs[i].count)continue;
			Level j = i + 3;
			if (j == 14)j = 1;
			else if (j == 15)j = 2;
			else if (j == 16)j = 0;
			else if (j == 17)j = -1;
			for (int k = 0; k < packs[i].count; k++)
				std::cout << j << " ";
		}
		std::cout << std::endl;
#endif
	}

	/**
	* 以下函数用于记录该副手牌中的各种合法牌型，相同牌型仅出现一次
	*/
	void getAllSingle()
	{
		single.clear();
		for (int i = 0; i <= 14; i++)
			if (packs[i].count)
				single.push_back(CardCombo(packs[i].card, packs[i].card + 1, CardComboType::SINGLE));
	}
	void getAllPair()
	{
		pair.clear();
		for (int i = 0; i <= 12; i++)
			if (packs[i].count >= 2)
				pair.push_back(CardCombo(packs[i].card, packs[i].card + 2, CardComboType::PAIR));
	}
	void getAllStraight()
	{
		for (int len = 5; len < 13; len++)
		{
			straight[len].clear();
			Card sequence[MAX_LEVEL];
			memset(sequence, -1, sizeof(sequence));
			for (int i = 0; i <= 11; i++)
				if (packs[i].count)
					sequence[i] = packs[i].card[0];
			for (int i = 0; i <= 7; i++)
			{
				int j = i;
				while (sequence[j] != -1 && j < i + len)j++;
				if (j == i + len)
					straight[len].push_back(CardCombo(sequence + i, sequence + j, CardComboType::STRAIGHT));
			}
		}
	}
	void getAllStraight2()
	{
		for (int len = 3; len < 11; len++)
		{
			straight2[len].clear();
			Card sequence[2 * MAX_LEVEL];
			memset(sequence, -1, sizeof(sequence));
			for (int i = 0; i <= 11; i++)
				if (packs[i].count >= 2)
				{
					sequence[2 * i] = packs[i].card[0];
					sequence[2 * i + 1] = packs[i].card[1];
				}
			for (int i = 0; i <= 18; i += 2)
			{
				int j = i;
				while (sequence[j] != -1 && j < i + len * 2)j++;
				if (j == i + len * 2)
					straight2[len].push_back(CardCombo(sequence + i, sequence + j, CardComboType::STRAIGHT2));
			}
		}
	}
	void getAllTriplet()
	{
		triplet.clear();
		for (int i = 0; i <= 12; i++)
			if (packs[i].count >= 3)
				triplet.push_back(CardCombo(packs[i].card, packs[i].card + 3, CardComboType::TRIPLET));
	}
	void getAllTriplet1()
	{
		triplet1.clear();
		for (int i = 0; i <= 12; i++)  // 主牌
		{
			if (packs[i].count < 3)continue;
			for (int j = 0; j <= 14; j++)  // 从牌
			{
				if (j == i)continue;
				if (packs[j].count)
				{
					vector<Card> solution(packs[i].card, packs[i].card + 3);
					solution.push_back(packs[j].card[0]);
					triplet1.push_back(CardCombo(solution.begin(), solution.end(), CardComboType::TRIPLET1));
				}
			}
		}
	}
	void getAllTriplet2()
	{
		triplet2.clear();
		for (int i = 0; i <= 12; i++)  // 主牌
		{
			if (packs[i].count < 3)continue;
			for (int j = 0; j <= 12; j++)  // 从牌
			{
				if (j == i)continue;
				if (packs[j].count >= 2)
				{
					vector<Card> solution(packs[i].card, packs[i].card + 3);
					solution.push_back(packs[j].card[0]);
					solution.push_back(packs[j].card[1]);
					triplet2.push_back(CardCombo(solution.begin(), solution.end(), CardComboType::TRIPLET2));
				}
			}
		}
	}
	void getAllBomb()
	{
		bomb.clear();
		for (int i = 0; i <= 12; i++)
			if (packs[i].count == 4)
				bomb.push_back(CardCombo(packs[i].card, packs[i].card + 4, CardComboType::BOMB));
	}
	void getAllQuadruple2()
	{
		quadruple2.clear();
		for (int i = 0; i <= 12; i++)  // 主牌
		{
			if (packs[i].count != 4)continue;
			for (int j = 0; j <= 14; j++)  // 从牌1
			{
				if (j == i || packs[j].count == 0)continue;
				for (int k = j + 1; k <= 14; k++)  // 从牌2
				{
					if (k == i || k == j || packs[k].count == 0)continue;
					vector<Card> solution(packs[i].card, packs[i].card + 4);
					solution.push_back(packs[j].card[0]);
					solution.push_back(packs[k].card[0]);
					quadruple2.push_back(CardCombo(solution.begin(), solution.end(), CardComboType::QUADRUPLE2));
				}
			}
		}
	}
	void getAllQuadruple4()
	{
		quadruple4.clear();
		for (int i = 0; i <= 12; i++)  // 主牌
		{
			if (packs[i].count < 4)continue;
			for (int j = 0; j <= 12; j++)  // 从牌1
			{
				if (j == i || packs[j].count < 2)continue;
				for (int k = j + 1; k <= 12; k++)  // 从牌2
				{
					if (k == i || k == j || packs[k].count < 2)continue;
					vector<Card> solution(packs[i].card, packs[i].card + 4);
					solution.push_back(packs[j].card[0]);
					solution.push_back(packs[j].card[1]);
					solution.push_back(packs[k].card[0]);
					solution.push_back(packs[k].card[1]);
					quadruple4.push_back(CardCombo(solution.begin(), solution.end(), CardComboType::QUADRUPLE4));
				}
			}
		}
	}
	void getAllPlane()
	{
		for (int len = 2; len < 7; len++)
		{
			plane[len].clear();
			for (int i = 0; i <= 10; i++)
			{
				if (packs[i].count < 3)continue;
				int j = i + 1;
				while (packs[j].count >= 3 && j < i + len)j++;
				if (j != i + len)continue;
				vector<Card> solution(packs[i].card, packs[i].card + 3);
				for (int k = i + 1; k < j; k++)
					solution.insert(solution.end(), packs[k].card, packs[k].card + 3);
				plane[len].push_back(CardCombo(solution.begin(), solution.end(), CardComboType::PLANE));
			}
		}
	}
	void getAllPlane1()
	{
		for (int len = 2; len <= 5; len++)
		{
			plane1[len].clear();
			for (int i = 0; i <= 10; i++)
			{
				if (packs[i].count < 3)continue;
				int j = i + 1;
				while (packs[j].count >= 3 && j < i + len)j++;
				if (j != i + len)continue;
				vector<Card> sequence(packs[i].card, packs[i].card + 3);
				for (int k = i + 1; k < j; k++)
					sequence.insert(sequence.end(), packs[k].card, packs[k].card + 3);
				vector<Card> solution(sequence.begin(), sequence.end());
				std::function<void(int, int)> f = [&](int w, int low)
				{
					if (w == 0)
					{
						plane1[len].push_back(CardCombo(solution.begin(), solution.end(), CardComboType::PLANE1));
						return;
					}
					for (int l = low; l <= 14; l++)
					{
						if (l >= i && l <= i + len - 1)continue;
						if (!packs[l].count)continue;
						solution.push_back(packs[l].card[0]);
						f(w - 1, l + 1);
						solution.pop_back();
					}
					return;
				};
				f(len, 0);
			}
		}
	}
	void getAllPlane2()
	{
		for (int len = 2; len <= 4; len++)
		{
			plane2[len].clear();
			for (int i = 0; i <= 10; i++)
			{
				if (packs[i].count < 3)continue;
				int j = i + 1;
				while (packs[j].count >= 3 && j < i + len)j++;
				if (j != i + len)continue;
				vector<Card> sequence(packs[i].card, packs[i].card + 3);
				for (int k = i + 1; k < j; k++)
					sequence.insert(sequence.end(), packs[k].card, packs[k].card + 3);
				vector<Card> solution(sequence.begin(), sequence.end());
				std::function<void(int, int)> f = [&](int w, int low)
				{
					if (w == 0)
					{
						plane2[len].push_back(CardCombo(solution.begin(), solution.end(), CardComboType::PLANE2));
						return;
					}
					for (int l = low; l <= 12; l++)
					{
						if (l >= i && l <= i + len - 1)continue;
						if (packs[l].count < 2)continue;
						solution.push_back(packs[l].card[0]);
						solution.push_back(packs[l].card[1]);
						f(w - 1, l + 1);
						solution.pop_back();
						solution.pop_back();
					}
					return;
				};
				f(len, 0);
			}
		}
	}
	void getAllSshuttle()
	{
		sshuttle.clear();
		for (int i = 0; i <= 10; i++)
		{
			if (packs[i].count != 4)continue;
			int j = i + 1;
			while (packs[j].count == 4 && j <= 11)j++;
			vector<Card> solution(packs[i].card, packs[i].card + 4);
			for (int k = i + 1; k < j; k++)
			{
				solution.insert(solution.end(), packs[k].card, packs[k].card + 4);
				sshuttle.push_back(CardCombo(solution.begin(), solution.end(), CardComboType::SSHUTTLE));
			}
		}
	}
	void getAllSshuttle2()
	{
		sshuttle2.clear();
		for (int i = 0; i <= 10; i++)
		{
			if (packs[i].count != 4)continue;
			int j = i + 1;
			while (packs[j].count == 4 && j <= 11)j++;
			vector<Card> sequence(packs[i].card, packs[i].card + 4);
			for (int k = i + 1; k < j; k++)
			{
				sequence.insert(sequence.end(), packs[k].card, packs[k].card + 4);
				int w = 2 * (k - i + 1);  // 需要的从牌数量
				if (w > 13 - k + i)break;  // 从牌种数不够了
				vector<Card> solution(sequence.begin(), sequence.end());
				std::function<void(int, int)> f = [&](int w, int low)
				{
					if (w == 0)
					{
						sshuttle2.push_back(CardCombo(solution.begin(), solution.end(), CardComboType::SSHUTTLE2));
						return;
					}
					for (int l = low; l <= 14; l++)
					{
						if (l >= i && l <= k)continue;
						if (!packs[l].count)continue;
						solution.push_back(packs[l].card[0]);
						f(w - 1, l + 1);
						solution.pop_back();
					}
					return;
				};
				f(w, 0);
			}
		}
	}
	void getAllSshuttle4()
	{
		sshuttle4.clear();
		for (int i = 0; i <= 10; i++)
		{
			if (packs[i].count != 4)continue;
			int j = i + 1;
			while (packs[j].count == 4 && j <= 11)j++;
			vector<Card> sequence(packs[i].card, packs[i].card + 4);
			for (int k = i + 1; k < j; k++)
			{
				sequence.insert(sequence.end(), packs[k].card, packs[k].card + 4);
				int w = 2 * (k - i + 1);  // 需要的从牌对数
				if (w > 11 - k + i)break;  // 从牌种数不够了
				vector<Card> solution(sequence.begin(), sequence.end());
				std::function<void(int, int)> f = [&](int w, int low)
				{
					if (w == 0)
					{
						sshuttle4.push_back(CardCombo(solution.begin(), solution.end(), CardComboType::SSHUTTLE4));
						return;
					}
					for (int l = low; l <= 12; l++)
					{
						if (l >= i && l <= k)continue;
						if (packs[l].count < 2)continue;
						solution.push_back(packs[l].card[0]);
						solution.push_back(packs[l].card[1]);
						f(w - 1, l + 1);
						solution.pop_back();
						solution.pop_back();
					}
					return;
				};
				f(w, 0);
			}
		}
	}
	void getAllRocket()
	{
		rocket.clear();
		if (packs[13].count && packs[14].count)
		{
			Card solution[2] = { 52, 53 };
			rocket.push_back(CardCombo(solution, solution + 2, CardComboType::ROCKET));
		}
	}
	void getAllCombos()
	{
		getAllSingle();
		getAllPair();
		getAllStraight();
		getAllStraight2();
		getAllTriplet();
		getAllTriplet1();
		getAllTriplet2();
		getAllBomb();
		getAllQuadruple2();
		getAllQuadruple4();
		getAllPlane();
		getAllPlane1();
		getAllPlane2();
		getAllSshuttle();
		getAllSshuttle2();
		getAllSshuttle4();
		getAllRocket();
	}

	/**
	* 输出当前手牌的所有合法牌型
	* 懒得改了x
	*/
	void printAllCombos()
	{
		getAllCombos();
		if (single.size())std::cout << "Single: " << std::endl << std::endl;
		for (CardCombo v : single)v.printCards();
		if (pair.size())std::cout << "Pair: " << std::endl << std::endl;
		for (CardCombo v : pair)v.printCards();
		// if (straight.size())std::cout << "Straight: " << std::endl << std::endl;
		// for (CardCombo v : straight)v.printCards();
		// if (straight2.size())std::cout << "Straight2: " << std::endl << std::endl;
		//for (CardCombo v : straight2)v.printCards();
		if (triplet.size())std::cout << "Triplet: " << std::endl << std::endl;
		for (CardCombo v : triplet)v.printCards();
		if (triplet1.size())std::cout << "Triplet1: " << std::endl << std::endl;
		for (CardCombo v : triplet1)v.printCards();
		if (triplet2.size())std::cout << "Triplet2: " << std::endl << std::endl;
		for (CardCombo v : triplet2)v.printCards();
		if (bomb.size())std::cout << "Bomb: " << std::endl << std::endl;
		for (CardCombo v : bomb)v.printCards();
		if (quadruple2.size())std::cout << "Quadruple2: " << std::endl << std::endl;
		for (CardCombo v : quadruple2)v.printCards();
		if (quadruple4.size())std::cout << "Quadruple4: " << std::endl << std::endl;
		for (CardCombo v : quadruple4)v.printCards();
		// if (plane.size())std::cout << "Plane: " << std::endl << std::endl;
		// for (CardCombo v : plane)v.printCards();
		// if (plane1.size())std::cout << "Plane1: " << std::endl << std::endl;
		// for (CardCombo v : plane1)v.printCards();
		// if (plane2.size())std::cout << "Plane2: " << std::endl << std::endl;
		// for (CardCombo v : plane2)v.printCards();
		if (sshuttle.size())std::cout << "Sshuttle: " << std::endl << std::endl;
		for (CardCombo v : sshuttle)v.printCards();
		if (sshuttle2.size())std::cout << "Sshuttle2: " << std::endl << std::endl;
		for (CardCombo v : sshuttle2)v.printCards();
		if (sshuttle4.size())std::cout << "Sshuttle4: " << std::endl << std::endl;
		for (CardCombo v : sshuttle4)v.printCards();
		if (rocket.size())std::cout << "Rocket: " << std::endl << std::endl;
		for (CardCombo v : rocket)v.printCards();
	}

	/**
	* 将[begin,end)区间中的牌从当前手牌中移除
	* 需确保当前手牌中有[begin,end)中的所有牌
	*/
	template <typename CARD_ITERATOR>
	void erase(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		for (CARD_ITERATOR i = begin; i != end; i++)
		{
			Level l = card2level(*i);
			packs[l].count--;
			int pos = 0;
			while (packs[l].card[pos] != *i)pos++;
			for (int j = pos; j < packs[l].count; j++)
				packs[l].card[j] = packs[l].card[j + 1];
		}
	}

	/**
	* 将[begin,end)区间中的牌加入当前手牌
	* 需确保当前手牌中没有[begin,end)中的任何牌
	*/
	template <typename CARD_ITERATOR>
	void insert(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		for (CARD_ITERATOR i = begin; i != end; i++)
		{
			Level l = card2level(*i);
			packs[l].card[packs[l].count] = *i;
			packs[l].count++;
		}
	}

	/**
	* 返回手牌共有多少张牌
	*/
	constexpr int getLength()
	{
		int length = 0;
		for (int i = 0; i <= 14; i++)length += packs[i].count;
		return length;
	}
};
#pragma endregion

#pragma region 读取输入，恢复状态，将决策写成JSON文件并输出
/**
* 按botzone的规则，每个回合，bot都是重新启动的；也就是说，我们无法记录之前的发牌和出牌历史；
* 因此，我们需要从输入的JSON文件中读取历史，将局面恢复到当前局面，再进行决策；
* （上传botzone的时候还可以选择长时模式，即回合结束后bot不会停止运行，但我室友说这个模式下输入输出容易出错，建议先按普通模式实现，再去尝试长时模式）
* 下面的代码就是用来读取输入、恢复状态、输出决策的；但是如何做出决策是由主函数决定的，下面的play函数仅仅是把决策写成JSON文件的形式并输出
*/

// 我的牌有哪些
set<Card> myCards;

// 地主明示的牌有哪些
set<Card> landlordPublicCards;

// 大家从最开始到现在都出过什么
vector<vector<Card>> whatTheyPlayed[PLAYER_COUNT];

// 当前要出的牌需要大过谁
CardCombo lastValidCombo;

// lastValidCombo是几号玩家出的
int lastValidPlayer;

// 大家还剩多少牌
short cardRemaining[PLAYER_COUNT] = { 17, 17, 17 };

// 我是几号玩家
int myPosition;

// 地主位置
int landlordPosition = -1;

// 地主叫分
int landlordBid = -1;

// 阶段
Stage stage = Stage::BIDDING;

// 自己的第一回合收到的叫分决策
vector<int> bidInput;

// 程序开始运行的时间，用于卡时
int startTime;

// 用于测试蒙特卡洛在本地和botzone上能模拟几局
int gamesSimulated = 0;

// 用于测试在Botzone上蒙特卡洛得分最高和第二高的决策相差多少分
int differenceBetweenFirstAndSecond = 0;

namespace BotzoneIO
{
	using namespace std;
	void read()
	{
		// 读入输入（平台上的输入是单行）
		string line;
		getline(cin, line);
		Json::Value input;
		Json::Reader reader;
		reader.parse(line, input);
		startTime = clock();

		// 首先处理第一回合，得知自己是谁、有哪些牌
		{
			auto firstRequest = input["requests"][0u]; // 下标需要是 unsigned，可以通过在数字后面加u来做到
			auto own = firstRequest["own"];
			for (unsigned i = 0; i < own.size(); i++)
				myCards.insert(own[i].asInt());
			if (!firstRequest["bid"].isNull())
			{
				// 如果还可以叫分，则记录叫分
				auto bidHistory = firstRequest["bid"];
				myPosition = bidHistory.size();
				for (unsigned i = 0; i < bidHistory.size(); i++)
					bidInput.push_back(bidHistory[i].asInt());
			}
		}

		// history里第一项（上上家）和第二项（上家）分别是谁的决策
		int whoInHistory[2] = {};

		int turn = input["requests"].size();
		for (int i = 0; i < turn; i++)
		{
			auto request = input["requests"][i];
			auto llpublic = request["publiccard"];
			if (!llpublic.isNull())
			{
				// 第一次得知公共牌、地主叫分和地主是谁
				landlordPosition = request["landlord"].asInt();
				landlordBid = request["finalbid"].asInt();
				myPosition = request["pos"].asInt();
				cardRemaining[landlordPosition] += llpublic.size();
				for (unsigned i = 0; i < llpublic.size(); i++)
				{
					landlordPublicCards.insert(llpublic[i].asInt());
					if (landlordPosition == myPosition)
						myCards.insert(llpublic[i].asInt());
				}
				whoInHistory[0] = (myPosition - 2 + PLAYER_COUNT) % PLAYER_COUNT;
				whoInHistory[1] = (myPosition - 1 + PLAYER_COUNT) % PLAYER_COUNT;
			}

			auto history = request["history"]; // 每个历史中有上家和上上家出的牌
			if (history.isNull())
				continue;
			stage = Stage::PLAYING;

			// 逐次恢复局面到当前
			int howManyPass = 0;
			for (int p = 0; p < 2; p++)
			{
				int player = whoInHistory[p];	// 是谁出的牌
				auto playerAction = history[p]; // 出的哪些牌
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // 循环枚举这个人出的所有牌
				{
					int card = playerAction[_].asInt(); // 这里是出的一张牌
					playedCards.push_back(card);
				}
				whatTheyPlayed[player].push_back(playedCards); // 记录这段历史
				cardRemaining[player] -= playerAction.size();

				if (playerAction.size() == 0)
					howManyPass++;
				else
					lastValidCombo = CardCombo(playedCards.begin(), playedCards.end());
			}

			if (howManyPass == 2)
				lastValidCombo = CardCombo();

			lastValidPlayer = (myPosition + 2 - howManyPass) % 3;

			if (i < turn - 1)
			{
				// 还要恢复自己曾经出过的牌
				auto playerAction = input["responses"][i]; // 出的哪些牌
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // 循环枚举自己出的所有牌
				{
					int card = playerAction[_].asInt(); // 这里是自己出的一张牌
					myCards.erase(card);				// 从自己手牌中删掉
					playedCards.push_back(card);
				}
				whatTheyPlayed[myPosition].push_back(playedCards); // 记录这段历史
				cardRemaining[myPosition] -= playerAction.size();
			}
		}
	}

	/**
	* 输出叫分（0, 1, 2, 3 四种之一）
	*/
	void bid(int value)
	{
		Json::Value result;
		result["response"] = value;

		Json::FastWriter writer;
		cout << writer.write(result) << endl;
	}

	/**
	* 输出打牌决策，begin是迭代器起点，end是迭代器终点
	* CARD_ITERATOR是Card（即short）类型的迭代器
	*/
	template <typename CARD_ITERATOR>
	void play(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		Json::Value result, response(Json::arrayValue);
		for (; begin != end; begin++)
			response.append(*begin);
		result["response"] = response;
		//result["debug"] = gamesSimulated;
		result["debug"] = differenceBetweenFirstAndSecond;
		Json::FastWriter writer;
		cout << writer.write(result) << endl;
	}
}
#pragma endregion

#pragma region 有用的函数
/**
* 输入一副手牌和需要大过的牌型，返回手牌中所有比lastValidCombo大的牌型（考虑了王炸、炸弹的情况）
*/
vector<CardCombo> getActions(CardCombinations& hand, const CardCombo& lastValidCombo)
{
	vector<CardCombo> actions;
	if (lastValidCombo.comboType != CardComboType::PASS)
		actions.push_back(CardCombo());
	switch (lastValidCombo.comboType)
	{
	case CardComboType::INVALID:
		return actions;
	case CardComboType::ROCKET:
		return actions;
	case CardComboType::BOMB:
		hand.getAllRocket();
		for (CardCombo rocket : hand.rocket)
			actions.push_back(rocket);
		hand.getAllBomb();
		for (CardCombo bomb : hand.bomb)
			if (bomb.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(bomb);
		return actions;
	case CardComboType::PASS:
		hand.getAllCombos();
		actions.insert(actions.end(), hand.single.begin(), hand.single.end());
		actions.insert(actions.end(), hand.pair.begin(), hand.pair.end());
		for (int len = 5; len <= 12; len++)
			actions.insert(actions.end(), hand.straight[len].begin(), hand.straight[len].end());
		for (int len = 3; len <= 10; len++)
			actions.insert(actions.end(), hand.straight2[len].begin(), hand.straight2[len].end());
		actions.insert(actions.end(), hand.triplet.begin(), hand.triplet.end());
		actions.insert(actions.end(), hand.triplet1.begin(), hand.triplet1.end());
		actions.insert(actions.end(), hand.triplet2.begin(), hand.triplet2.end());
		actions.insert(actions.end(), hand.bomb.begin(), hand.bomb.end());
		actions.insert(actions.end(), hand.quadruple2.begin(), hand.quadruple2.end());
		actions.insert(actions.end(), hand.quadruple4.begin(), hand.quadruple4.end());
		for (int len = 2; len <= 6; len++)
			actions.insert(actions.end(), hand.plane[len].begin(), hand.plane[len].end());
		for (int len = 2; len <= 5; len++)
			actions.insert(actions.end(), hand.plane1[len].begin(), hand.plane1[len].end());
		for (int len = 2; len <= 4; len++)
			actions.insert(actions.end(), hand.plane2[len].begin(), hand.plane2[len].end());
		actions.insert(actions.end(), hand.sshuttle.begin(), hand.sshuttle.end());
		actions.insert(actions.end(), hand.sshuttle2.begin(), hand.sshuttle2.end());
		actions.insert(actions.end(), hand.sshuttle4.begin(), hand.sshuttle4.end());
		actions.insert(actions.end(), hand.rocket.begin(), hand.rocket.end());
		return actions;
	}
	// 先把炸弹和王炸加到可行的策略里
	hand.getAllRocket();
	for (CardCombo rocket : hand.rocket)
		actions.push_back(rocket);
	hand.getAllBomb();
	for (CardCombo bomb : hand.bomb)
		actions.push_back(bomb);
	// 再分类讨论
	short length;
	switch (lastValidCombo.comboType)
	{
	case CardComboType::SINGLE:
		hand.getAllSingle();
		for (CardCombo single : hand.single)
			if (single.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(single);
		return actions;
	case CardComboType::PAIR:
		hand.getAllPair();
		for (CardCombo pair : hand.pair)
			if (pair.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(pair);
		return actions;
	case CardComboType::STRAIGHT:
		length = lastValidCombo.cards.size();
		hand.getAllStraight();
		for (int len = 5; len <= 12; len++)
			for (CardCombo straight : hand.straight[len])
				if (straight.cards.size() == length && straight.comboLevel > lastValidCombo.comboLevel)
					actions.push_back(straight);
		return actions;
	case CardComboType::STRAIGHT2:
		length = lastValidCombo.cards.size();
		hand.getAllStraight2();
		for (int len = 3; len <= 10; len++)
			for (CardCombo straight2 : hand.straight2[len])
				if (straight2.cards.size() == length && straight2.comboLevel > lastValidCombo.comboLevel)
					actions.push_back(straight2);
		return actions;
	case CardComboType::TRIPLET:
		hand.getAllTriplet();
		for (CardCombo triplet : hand.triplet)
			if (triplet.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(triplet);
		return actions;
	case CardComboType::TRIPLET1:
		hand.getAllTriplet1();
		for (CardCombo triplet1 : hand.triplet1)
			if (triplet1.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(triplet1);
		return actions;
	case CardComboType::TRIPLET2:
		hand.getAllTriplet2();
		for (CardCombo triplet2 : hand.triplet2)
			if (triplet2.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(triplet2);
		return actions;
	case CardComboType::QUADRUPLE2:
		hand.getAllQuadruple2();
		for (CardCombo quadruple2 : hand.quadruple2)
			if (quadruple2.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(quadruple2);
		return actions;
	case CardComboType::QUADRUPLE4:
		hand.getAllQuadruple4();
		for (CardCombo quadruple4 : hand.quadruple4)
			if (quadruple4.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(quadruple4);
		return actions;
	case CardComboType::PLANE:
		length = lastValidCombo.cards.size();
		hand.getAllPlane();
		for (int len = 2; len <= 6; len++)
			for (CardCombo plane : hand.plane[len])
				if (plane.cards.size() == length && plane.comboLevel > lastValidCombo.comboLevel)
					actions.push_back(plane);
		return actions;
	case CardComboType::PLANE1:
		length = lastValidCombo.cards.size();
		hand.getAllPlane1();
		for (int len = 2; len <= 5; len++)
			for (CardCombo plane1 : hand.plane1[len])
				if (plane1.cards.size() == length && plane1.comboLevel > lastValidCombo.comboLevel)
					actions.push_back(plane1);
		return actions;
	case CardComboType::PLANE2:
		length = lastValidCombo.cards.size();
		hand.getAllPlane2();
		for (int len = 2; len <= 4; len++)
			for (CardCombo plane2 : hand.plane2[len])
				if (plane2.cards.size() == length && plane2.comboLevel > lastValidCombo.comboLevel)
					actions.push_back(plane2);
		return actions;
	case CardComboType::SSHUTTLE:
		length = lastValidCombo.cards.size();
		hand.getAllSshuttle();
		for (CardCombo sshuttle : hand.sshuttle)
			if (sshuttle.cards.size() == length && sshuttle.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(sshuttle);
		return actions;
	case CardComboType::SSHUTTLE2:
		length = lastValidCombo.cards.size();
		hand.getAllSshuttle2();
		for (CardCombo sshuttle2 : hand.sshuttle2)
			if (sshuttle2.cards.size() == length && sshuttle2.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(sshuttle2);
		return actions;
	case CardComboType::SSHUTTLE4:
		length = lastValidCombo.cards.size();
		hand.getAllSshuttle4();
		for (CardCombo sshuttle4 : hand.sshuttle4)
			if (sshuttle4.cards.size() == length && sshuttle4.comboLevel > lastValidCombo.comboLevel)
				actions.push_back(sshuttle4);
		return actions;
	}
	return actions;
}

/**
* 依次输出由CardCombo组成的vector中所有CardCombo的牌型
* 用于上一个函数的debug
*/
void printActions(vector<CardCombo>& actions)
{
	for (CardCombo action : actions)
		action.printCards();
}
#pragma endregion

#pragma region 一个改进估值函数和策略的AI
int getUtility(CardCombinations& hand, int type, const int alpha);

/**
* 输入一副手牌，返回这副牌的估值utility(Hand, alpha), alpha为可变参数
* 估值函数的思路是：用深搜把手牌拆成许多牌型，每一种拆法对应一个utility，最终返回其最大值，也就是对应的最佳拆法
* 估值函数越大，意味着(1)手牌的压制力越大；(2)手牌越快能出完。在不同的时候，(1)和(2)的权重是不同的。
* 用alpha来控制权重。大约有最终的估值 = 大牌的数量和质量 - alpha * 出完的手数。
* alpha越小，越重视大牌质量；alpha越大，越重视速度
* 拆牌时考虑的顺序为：火箭、炸弹、飞机、连对、顺子、三带、对子、单张。什么四带二？航天飞机？没听说过啊www
*/
int utility(CardCombinations& hand, const int alpha = 10)
{
	return getUtility(hand, 0, alpha);
}

int getUtility(CardCombinations& hand, int type, const int alpha)
{
	int utility1 = 0, utility2 = 0;	//分别对应把牌型直接打出和拆掉牌型两种策略
	int maxutility;
	switch (type)
	{
	case 0:
		hand.getAllRocket();
		if (!hand.rocket.size())
			return getUtility(hand, 1, alpha);
		else
		{
			CardCombo myRocket = *hand.rocket.begin();
			hand.erase(myRocket.cards.begin(), myRocket.cards.end());
			utility1 = getUtility(hand, 1, alpha) + 15;
			hand.insert(myRocket.cards.begin(), myRocket.cards.end());
			utility2 = getUtility(hand, 1, alpha);
			return std::max(utility1, utility2);
			break;
		}
	case 1:
		hand.getAllBomb();
		if (!hand.bomb.size())	//没有炸弹，直接判断下一种牌型
			return getUtility(hand, 2, alpha);
		else
		{
			CardCombo biggestBomb = *hand.bomb.begin();
			hand.erase(biggestBomb.cards.begin(), biggestBomb.cards.end());	//删掉最大的炸弹之后递归
			utility1 = getUtility(hand, 1, alpha) + biggestBomb.comboLevel;	//炸弹比较特殊，由于不仅可以随便出还能让自己多走一手牌，所以是+alpha而非-alpha
			hand.insert(biggestBomb.cards.begin(), biggestBomb.cards.end());
			utility2 = getUtility(hand, 2, alpha);	//当然也可以选择把炸弹拆开出
			return std::max(utility1, utility2);
		}
		break;
	case 2:
		hand.getAllPlane();
		hand.getAllPlane1();
		hand.getAllPlane2();
		if (!hand.plane[2].size())
			return getUtility(hand, 3, alpha);
		else
		{
			maxutility = -233333;
			for (int len = 2; len <= 6; len++)
			{
				auto planeCopy(hand.plane[len]);
				for (auto planei = planeCopy.begin(); planei != planeCopy.end(); planei++)
				{
					hand.erase(planei->cards.begin(), planei->cards.end());
					utility1 = getUtility(hand, 2, alpha) + planei->comboLevel - len + 1 - alpha;
					if (hand.plane[len].size() > 1)
						utility1 += 2;
					maxutility = std::max(maxutility, utility1);
					hand.insert(planei->cards.begin(), planei->cards.end());
				}
				auto plane1Copy(hand.plane1[len]);
				for (auto plane1i = plane1Copy.begin(); plane1i != plane1Copy.end(); plane1i++)
				{
					hand.erase(plane1i->cards.begin(), plane1i->cards.end());
					utility1 = getUtility(hand, 2, alpha) + plane1i->comboLevel - len + 1 - alpha;
					if (hand.plane1[len].size() > 1)
						utility1 += 2;
					maxutility = std::max(maxutility, utility1);
					hand.insert(plane1i->cards.begin(), plane1i->cards.end());
				}
				auto plane2Copy(hand.plane2[len]);
				for (auto plane2i = plane2Copy.begin(); plane2i != plane2Copy.end(); plane2i++)
				{
					hand.erase(plane2i->cards.begin(), plane2i->cards.end());
					utility1 = getUtility(hand, 2, alpha) + plane2i->comboLevel - len + 1 - alpha;
					if (hand.plane2[len].size() > 1)
						utility1 += 2;
					maxutility = std::max(maxutility, utility1);
					hand.insert(plane2i->cards.begin(), plane2i->cards.end());
				}
			}
			utility2 = getUtility(hand, 3, alpha);
			return std::max(maxutility, utility2);
		}
		break;
	case 3:
		hand.getAllStraight2();
		if (!hand.straight2[3].size())
			return getUtility(hand, 4, alpha);
		else
		{
			maxutility = -233333;
			for (int len = 3; len <= 10; len++)
			{
				auto straight2Copy(hand.straight2[len]);
				for (auto straight2i = straight2Copy.begin(); straight2i != straight2Copy.end(); straight2i++)
				{
					hand.erase(straight2i->cards.begin(), straight2i->cards.end());
					utility1 = getUtility(hand, 3, alpha) + straight2i->comboLevel - len + 1 - alpha;
					if (hand.straight2[len].size() > 1)
						utility1 += 2;
					if (straight2i->comboLevel == 11)
						utility1++;
					maxutility = std::max(maxutility, utility1);
					hand.insert(straight2i->cards.begin(), straight2i->cards.end());
				}
			}
			utility2 = getUtility(hand, 4, alpha);
			return std::max(maxutility, utility2);
		}
		break;
	case 4:
		hand.getAllStraight();
		if (!hand.straight[5].size())
			return getUtility(hand, 5, alpha);
		else
		{
			maxutility = -233333;
			for (int len = 5; len <= 12; len++)
			{
				auto straightCopy(hand.straight[len]);
				for (auto straighti = straightCopy.begin(); straighti != straightCopy.end(); straighti++)
				{
					hand.erase(straighti->cards.begin(), straighti->cards.end());
					utility1 = getUtility(hand, 4, alpha) + straighti->comboLevel - len + 1 - alpha;
					if (hand.straight[len].size() > 1)
						utility1 += 2;
					if (straighti->comboLevel == 11)
						utility1++;
					maxutility = std::max(maxutility, utility1);
					hand.insert(straighti->cards.begin(), straighti->cards.end());
				}
			}
			utility2 = getUtility(hand, 5, alpha);
			return std::max(maxutility, utility2);
		}
		break;
	case 5:
		hand.getAllSingle();	//为了加快速度起见，单张、对子、三张一起考虑
		for (auto singlei = hand.single.begin(); singlei != hand.single.end(); singlei++)
		{
			utility1 += singlei->comboLevel + hand.packs[singlei->comboLevel].count - alpha;	//多张比单张相对难管一些，所以估值增加
			if (singlei->comboLevel >= 11)	//A, 2, 小王, 大王比较强，有额外加成
				utility1 += hand.packs[singlei->comboLevel].count * hand.packs[singlei->comboLevel].count * (singlei->comboLevel - 10);
		}
		return utility1;
		break;
	default:
		std::cout << '?' << std::endl;
		return 233333333;
		break;
	}
}

set<Card> remainingCards;
/**
 * 返回其他两人手中炸弹和火箭的最大可能数量
 * 按理说如果外面有炸弹的时候自己出牌应该谨慎一些，不无脑出大牌和炸弹的
 * 但具体咋写我还没想好，所以这个函数暂时没有在决策函数里面出现
 */
int howManyBombsOutside()
{
	CardCombinations outsideCards(remainingCards.begin(), remainingCards.end());
	outsideCards.getAllBomb();
	outsideCards.getAllRocket();
	return outsideCards.bomb.size() + outsideCards.rocket.size();
}

/**
* 输入手牌和本次出牌要大过的牌，以及可变参数alpha, beta，返回要出的牌
* 搜索思路是：遍历所有能出的出牌方式，选择使(10 * utility增大值 + beta * 牌力)最大的一种。
* 显然，beta越大，越倾向于压牌。而beta足够小时ai会选择pass（除非自己主动），因为强行压牌可能会让utility减小，但pass不会
* 特别地，如果自己是地主下家且地主上家报单，自己主动时会出最小的单牌。
*/
CardCombo _findBestAction(CardCombinations& hand, const CardCombo& lastValidCombo, int alpha = 19, const int beta = 6)
{
	vector<CardCombo> actions = getActions(hand, lastValidCombo);
	if (actions.size() == 0)return CardCombo();
	CardCombo bestAction = CardCombo();
	int bestMeasure = lastValidCombo.cards.size() ? 0 : -233333;	//如果自己主动则不能跳过，否则可以
	if (!lastValidCombo.cards.size())	//如果自己主动的话会更倾向于速度
		alpha += 3;
	int originalUtility = utility(hand, alpha);
	int tempMeasure;
	for (CardCombo action : actions)
	{
		hand.erase(action.cards.begin(), action.cards.end());
		if (!hand.getLength())	//出完啦！（即使是四个2带俩王也要全甩出去x）
		{
			hand.insert(action.cards.begin(), action.cards.end());
			return action;
		}
		tempMeasure = 10 * (utility(hand, alpha) - originalUtility) + beta * action.comboLevel;
		if (action.comboType == CardComboType::BOMB || action.comboType == CardComboType::ROCKET)
			tempMeasure += beta * 6;	//这个数值表示对炸的偏好程度，炸的目的是alpha: 尽早打完牌 beta:卡别人
		else if (action.comboLevel >= 11)	//A, 2, 小王, 大王的强度有加成
			tempMeasure += beta * (action.comboLevel - 10 + 3 * (action.comboLevel >= 13));
		hand.insert(action.cards.begin(), action.cards.end());
		if (tempMeasure > bestMeasure)
		{
			bestMeasure = tempMeasure;
			bestAction = action;
		}
	}
	if (myPosition == (landlordPosition + 1) % 3 && cardRemaining[(myPosition + 1) % 3] == 1 && !lastValidCombo.cards.size())
	{
		hand.getAllSingle();
		return CardCombo(hand.single.begin()->cards.begin(), hand.single.begin()->cards.end(), CardComboType::SINGLE);
	}
	//std::cout << "alpha = " << alpha << ", beta = " << beta << ", bestMeasure = " << bestMeasure << std::endl;
	return bestAction;
}

CardCombo findBestAction(CardCombinations& hand, const CardCombo& lastValidCombo, int position, int landlordPosition)
{
	int alpha = 19, beta = 6;
	if (position == (landlordPosition + 2) % 3 && lastValidPlayer == landlordPosition)	//众所周知地主上家要压牌
		beta += 5;
	if (position == (landlordPosition + 2) % 3 && lastValidPlayer == (landlordPosition + 1) % 3 && lastValidCombo.comboLevel >= 9)	//地主下家打了大牌，那地主上家不要
		beta -= lastValidCombo.comboLevel - 8;
	if (position == (landlordPosition + 1) % 3 && lastValidPlayer == (landlordPosition + 2) % 3)	//地主上家打牌地主不要，那地主下家也不要
		beta -= 5;
	return _findBestAction(hand, lastValidCombo, alpha, beta);
}

/**
* 返回最好的n个决策（当前可行的action不足n个的话就返回所有可行的actions），原理与上一个函数一样
*/
vector<CardCombo> _findBestNActions(int _n, CardCombinations& hand, const CardCombo& lastValidCombo, int alpha = 19, const int beta = 6)
{
	vector<CardCombo> actions = getActions(hand, lastValidCombo);
	int n = std::min(_n, int(actions.size()));
	vector<CardCombo> bestActions;
	std::multimap<int, CardCombo> actionsScores;
	if (!lastValidCombo.cards.size())	//如果自己主动的话会更倾向于速度
		alpha += 3;
	int originalUtility = utility(hand, alpha);
	int tempMeasure;
	for (CardCombo action : actions)
	{
		hand.erase(action.cards.begin(), action.cards.end());
		tempMeasure = 10 * (utility(hand, alpha) - originalUtility) + beta * action.comboLevel;
		if (action.comboType == CardComboType::BOMB || action.comboType == CardComboType::ROCKET)
			tempMeasure += beta * 6;	//这个数值表示对炸的偏好程度，炸的目的是alpha: 尽早打完牌 beta:卡别人
		else if (action.comboLevel >= 11)	//A, 2, 小王, 大王的强度有加成
			tempMeasure += beta * (action.comboLevel - 9 + (action.comboLevel >= 13));
		hand.insert(action.cards.begin(), action.cards.end());
		actionsScores.insert(std::make_pair(tempMeasure, action));
	}
	std::reverse_iterator<std::multimap<int, CardCombo>::iterator> i = actionsScores.rbegin();
	for (int j = 0; j < n; j++) { bestActions.push_back(i->second); i++; }
	//if (bestActions.size() < _n)bestActions.push_back(CardCombo());
	return bestActions;
}

/**
 * 手动加了一些逻辑判断
 */
vector<CardCombo> findBestNActions(int _n, CardCombinations& hand, const CardCombo& lastValidCombo, int position, int landlordPosition)
{
	int alpha = 19, beta = 6;
	int enemyLeft = 0;
	if (position == landlordPosition)
		enemyLeft = std::min(cardRemaining[(position + 1) % 3], cardRemaining[(position + 2) % 3]);
	else
		enemyLeft = cardRemaining[landlordPosition];
	if (enemyLeft <= 5)	//对手快要出完了！
		beta += 6 - enemyLeft;
	if (position == (landlordPosition + 2) % 3 && lastValidPlayer == landlordPosition)	//众所周知地主上家要压牌
		beta += 5;
	if (position == (landlordPosition + 2) % 3 && lastValidPlayer == (landlordPosition + 1) % 3 && lastValidCombo.comboLevel >= 9)	//地主下家打了大牌，那地主上家不要
		beta -= lastValidCombo.comboLevel - 8;
	if (position == (landlordPosition + 1) % 3 && lastValidPlayer == (landlordPosition + 2) % 3)	//地主上家打牌地主不要，那地主下家也不要
		beta -= 5;
	return _findBestNActions(_n, hand, lastValidCombo, alpha, beta);
}
#pragma endregion

#pragma region 蒙特卡洛树搜索
//set<Card> remainingCards;	前面有个函数要用，就在前面定义了
CardCombinations myHand;  // 我的手牌
CardCombinations nextHand;  // 下家的手牌
CardCombinations previousHand;  // 上家的手牌
vector<CardCombo> rootActions;  // 我的可行的决策（即findBestNActions()返回的n个决策）
std::map<CardCombo, int> winningTimes;  // 最终每个决策的totalScore之和
int totalScore = 1;  // 每次模拟对局的最终得分，默认为1，若有炸弹、火箭、春天、反春的情况则翻倍
bool landlordHasNotPlayed = true;  // 地主到现在为止，除了第一手牌，有没有出过牌
bool landlordHasNotPlayed_;  // 在每局模拟对局中记录地主除了第一手牌有没有出过牌

struct TreeNode
{
	int turn;  // which player's turn, i.e., who I am
	CardCombinations hands[PLAYER_COUNT];  // each player's hand
	TreeNode() {}
	TreeNode(int _turn, CardCombinations _player0, CardCombinations _player1, CardCombinations _player2, bool _isLeaf = false)
	{
		turn = _turn;
		hands[0] = _player0;
		hands[1] = _player1;
		hands[2] = _player2;
	}
}root;  // 蒙特卡洛树节点
std::multimap<CardCombo, TreeNode*> sons;  // 我的决策和对应的节点一一对应构成的map

/**
* 准备蒙特卡洛模拟需要的数据，将myHand、remainingCards、root.hands[myPosition]、root.turn、rootActions记录下来，并初始化winningTimes
*/
void prepareData()
{
	myHand = CardCombinations(myCards.begin(), myCards.end());
	for (Card i = 0; i <= 53; i++)remainingCards.insert(i);
	for (Card card : myCards)remainingCards.erase(card);
	for (int i = 0; i < 3; i++)
		for (vector<Card> vc : whatTheyPlayed[i])
			for (Card v : vc)
				remainingCards.erase(v);
	root.hands[myPosition] = myHand;
	root.turn = myPosition;
	rootActions = findBestNActions(3, myHand, lastValidCombo, myPosition, landlordPosition);
	for (CardCombo action : rootActions)
		winningTimes.insert(std::make_pair(action, 0));
	if (whatTheyPlayed[landlordPosition].size())
		for (vector<vector<Card> >::iterator i = whatTheyPlayed[landlordPosition].begin() + 1; i != whatTheyPlayed[landlordPosition].end(); i++)
			if (i->size())landlordHasNotPlayed = false;
}

/**
* 将remainingCards随机打乱分给其他两个玩家，并将数据记录在root.hands和sons中
*/
void determinization(int nextPlayerRemainingCards)
{
	srand(rand());
	vector<Card> a(remainingCards.begin(), remainingCards.end());
	std::random_shuffle(a.begin(), a.end());
	nextHand = CardCombinations(a.begin(), a.begin() + nextPlayerRemainingCards);
	previousHand = CardCombinations(a.begin() + nextPlayerRemainingCards, a.end());
	switch (myPosition)
	{
	case 0:
		root.hands[1] = nextHand;
		root.hands[2] = previousHand;
		break;
	case 1:
		root.hands[0] = previousHand;
		root.hands[2] = nextHand;
		break;
	case 2:
		root.hands[0] = nextHand;
		root.hands[1] = previousHand;
		break;
	}
	sons.clear();
	for (CardCombo action : rootActions)
	{
		root.hands[myPosition].erase(action.cards.begin(), action.cards.end());
		sons.insert(std::make_pair(action, new TreeNode((myPosition + 1) % 3, root.hands[0], root.hands[1], root.hands[2], true)));
		root.hands[myPosition].insert(action.cards.begin(), action.cards.end());
	}
	/*for (int i = 0; i < 3; i++)
	{
		std::cout << "player " << i << ":";
		root.hands[i].printCards();
	}*/
}

/**
* 进行模拟对局，返回本次对局的totalScore
* 模拟对局的逻辑是：在当前玩家出牌之后的模拟中，每个玩家都按照findBestAction()返回的决策出牌
* 底分为1，若遇炸弹、火箭、春天、反春的情形则低分翻倍（当然每局对局开始时底分totalScore要恢复为1）
* 最后若当前玩家获胜，返回最终得分totalScore；否则，返回-toalScore
* 对当前玩家最好的3个决策进行多次模拟，最终选择totalScore之和最大的决策作为出牌决策
*/
int MCTS(TreeNode* _node, const CardCombo& _lastValidCombo, const int _lastValidPlayer)
{
	int turn = _node->turn;  // whose turn to play
	if (!_node->hands[(turn + 2) % 3].getLength())  // game over
	{
		int winner = (turn + 2) % 3;
		/*std::cout << "over, ";
		std::cout << "winner: " << winner << ", ";
		std::cout << "totalScore: " << totalScore << std::endl;*/
		if (winner == landlordPosition && _node->hands[(landlordPosition + 1) % 3].getLength() == 17
			&& _node->hands[(landlordPosition + 2) % 3].getLength() == 17)
			totalScore *= 2;  // landlord spring
		else if (winner != landlordPosition && landlordHasNotPlayed_)
			totalScore *= 2;  // farmer spring
		if (winner == myPosition)return totalScore;  // win
		else if (myPosition != landlordPosition && winner != landlordPosition)return totalScore;  // the other farmer wins
		return -totalScore;
	}
	CardCombo action = findBestAction(_node->hands[turn], _lastValidCombo, turn, landlordPosition);
	/*std::cout << turn << ": ";
	action.printCards();*/
	CardCombo lastValidCombo_ = action;
	int lastValidPlayer_ = turn;
	if (turn == landlordPosition && action.cards.size())landlordHasNotPlayed_ = false;
	if (action.comboType == CardComboType::PASS)
	{
		lastValidPlayer_ = _lastValidPlayer;
		if (_lastValidPlayer == (turn + 2) % 3)
			lastValidCombo_ = _lastValidCombo;
	}
	else if (action.comboType == CardComboType::BOMB || action.comboType == CardComboType::ROCKET)totalScore *= 2;
	_node->hands[turn].erase(action.cards.begin(), action.cards.end());
	TreeNode* newNode = new TreeNode((turn + 1) % 3, _node->hands[0], _node->hands[1], _node->hands[2]);
	_node->hands[turn].insert(action.cards.begin(), action.cards.end());
	return MCTS(newNode, lastValidCombo_, lastValidPlayer_);
}

/**
* 在timeLimit时间范围内进行尽可能多的模拟对局，将每种决策的totalScore之和算出来
*/
void calculateScores(int timeLimit = 950)
{
	while ((clock() - startTime) * 1000 < timeLimit * CLOCKS_PER_SEC)
	{
		determinization(cardRemaining[(myPosition + 1) % 3]);
		for (std::pair<CardCombo, TreeNode*> son : sons)
		{
			CardCombo lastValidCombo_ = son.first;
			int lastValidPlayer_ = myPosition;
			if (son.first.comboType == CardComboType::PASS)
			{
				lastValidPlayer_ = lastValidPlayer;
				if (lastValidPlayer == (myPosition + 2) % 3)
					lastValidCombo_ = lastValidCombo;
			}
			totalScore = 1;
			if (lastValidCombo_.comboType == CardComboType::BOMB || lastValidCombo_.comboType == CardComboType::ROCKET)
				totalScore *= 2;
			landlordHasNotPlayed_ = landlordHasNotPlayed;
			if (myPosition == landlordPosition && lastValidCombo.cards.size())landlordHasNotPlayed_ = false;
			/*std::cout << myPosition << ": ";
			son.first.printCards();*/
			winningTimes[son.first] += MCTS(son.second, lastValidCombo_, lastValidPlayer_);
			if ((clock() - startTime) * 1000 >= timeLimit * CLOCKS_PER_SEC)break;
			//gamesSimulated++;
		}
	}
	/*for (std::pair<CardCombo, int> score : winningTimes)
	{
	   std::cout << score.second << " for the following action" << std::endl;
	   score.first.printCards();
	}*/
}

/**
* 返回totalScore之和最大的决策
*/
CardCombo returnAction()
{
	calculateScores();
	int bestWinningTimes = INT_MIN;
	CardCombo bestAction = *rootActions.begin();
	for (std::pair<CardCombo, int> p : winningTimes)
		if (p.second >= bestWinningTimes)
		{
			bestWinningTimes = p.second;
			bestAction = p.first;
		}
	//return bestAction;
	winningTimes.erase(bestAction);
	if (!winningTimes.size())return bestAction;
	int secondBestWinningTimes = INT_MIN;
	for (std::pair<CardCombo, int> p : winningTimes)
		if (p.second >= secondBestWinningTimes)
			secondBestWinningTimes = p.second;
	differenceBetweenFirstAndSecond = bestWinningTimes - secondBestWinningTimes;
	//std::cout << "differenceBetweenFirstAndSecond: " << differenceBetweenFirstAndSecond << std::endl;
	if (bestWinningTimes - secondBestWinningTimes >= 40)return bestAction;
	else if (bestWinningTimes - winningTimes[*rootActions.begin()] >= 50)return bestAction;
	return *rootActions.begin();
}
#pragma endregion

int main()
{
	BotzoneIO::read();
	prepareData();  // 记录myHand、remainingCards

	if (stage == Stage::BIDDING)
	{
#pragma region 测试对手牌的估值
		// CardCombinations myHand(myCards.begin(), myCards.end());
		// std::cout << std::endl;
		// std::cout << "myCards in number: ";
		// int numbers[21] = {};
		// for (set<Card>::iterator i = myCards.begin(); i != myCards.end(); i++)
		// {
		// 	int j = card2level(*i) + 3;
		// 	if (j == 14)j = 1;
		// 	else if (j == 15)j = 2;
		// 	else if (j == 16)j = 0;
		// 	else if (j == 17)j = -1;
		// 	numbers[++numbers[0]] = j;
		// }
		// std::copy(numbers + 1, numbers + numbers[0] + 1, std::ostream_iterator<int>(std::cout, " "));
		// std::cout << std::endl << std::endl;
		// std::cout << "utility of myCards: " << std::endl;
		// for (int alpha = 1; alpha < 20; alpha++)
		// 	std::cout << "alpha = " << alpha << ", utility = " << utility(myHand, alpha) << std::endl;
#pragma endregion
		// 做出决策（你只需修改以下部分）

		int bidValue = utility(myHand, 10) >= 0 ? 3 : 0;	//这个条件很松，而且只要叫地主必是3分（x
		//int bidValue = 3;
		// 决策结束，输出结果（你只需修改以上部分）

		BotzoneIO::bid(bidValue);
	}
	else if (stage == Stage::PLAYING)
	{
		// 做出决策（你只需修改以下部分）;

		CardCombo myAction = !whatTheyPlayed[myPosition].size() ? findBestAction(myHand, lastValidCombo, myPosition, landlordPosition)
			: returnAction();
		if (!myAction.cards.size())myAction = *rootActions.begin();

#pragma region 测试出牌情况
		// std::cout << std::endl;
		// std::cout << "myCards in number: ";
		// int numbers[21] = {};
		// for (set<Card>::iterator i = myCards.begin(); i != myCards.end(); i++)
		// {
		// 	int j = card2level(*i) + 3;
		// 	if (j == 14)j = 1;
		// 	else if (j == 15)j = 2;
		// 	else if (j == 16)j = 0;
		// 	else if (j == 17)j = -1;
		// 	numbers[++numbers[0]] = j;
		// }
		// copy(numbers + 1, numbers + numbers[0] + 1, std::ostream_iterator<int>(std::cout, " "));
		// std::cout << std::endl << std::endl;
		// std::cout << "lastValidCombo: ";
		// memset(numbers, 0, 21);
		// for (vector<Card>::iterator i = lastValidCombo.cards.begin(); i != lastValidCombo.cards.end(); i++)
		// {
		// 	int j = card2level(*i) + 3;
		// 	if (j == 14)j = 1;
		// 	else if (j == 15)j = 2;
		// 	else if (j == 16)j = 0;
		// 	else if (j == 17)j = -1;
		// 	numbers[++numbers[0]] = j;
		// }
		// copy(numbers + 1, numbers + numbers[0] + 1, std::ostream_iterator<int>(std::cout, " "));
		// std::cout << std::endl << std::endl;
		// for (int alpha = 5; alpha <= 15; alpha++)
		// 	for (int beta = 0; beta <= 15; beta++)
		// 	{
		// 		CardCombo myAction = findBestAction(myHand, lastValidCombo, alpha, beta);
		// 		memset(numbers, 0, 21);
		// 		for (vector<Card>::iterator i = myAction.cards.begin(); i != myAction.cards.end(); i++)
		// 		{
		// 			int j = card2level(*i) + 3;
		// 			if (j == 14)j = 1;
		// 			else if (j == 15)j = 2;
		// 			else if (j == 16)j = 0;
		// 			else if (j == 17)j = -1;
		// 			numbers[++numbers[0]] = j;
		// 		}
		// 		copy(numbers + 1, numbers + numbers[0] + 1, std::ostream_iterator<int>(std::cout, " "));
		// 		std::cout << std::endl;
		// 	}
#pragma endregion
#pragma region 输出当前状态相关变量
		/*std::cout << std::endl;
		std::cout << "myCards in number: ";
		int numbers[21] = {};
		for (set<Card>::iterator i = myCards.begin(); i != myCards.end(); i++)
		{
			int j = card2level(*i) + 3;
			if (j == 14)j = 1;
			else if (j == 15)j = 2;
			else if (j == 16)j = 0;
			else if (j == 17)j = -1;
			numbers[++numbers[0]] = j;
		}
		copy(numbers + 1, numbers + numbers[0] + 1, std::ostream_iterator<int>(std::cout, " "));
		std::cout << std::endl << std::endl;

		std::cout << "landlordPublicCards: ";
		memset(numbers, 0, 21);
		for (set<Card>::iterator i = landlordPublicCards.begin(); i != landlordPublicCards.end(); i++)
		{
			int j = card2level(*i) + 3;
			if (j == 14)j = 1;
			else if (j == 15)j = 2;
			else if (j == 16)j = 0;
			else if (j == 17)j = -1;
			numbers[++numbers[0]] = j;
		}
		copy(numbers + 1, numbers + numbers[0] + 1, std::ostream_iterator<int>(std::cout, " "));
		std::cout << std::endl << std::endl;

		for (int k = 0; k < 3; k++)
		{
			std::cout << "whatTheyPlayed " << k << ": " << std::endl;
			for (vector<Card> v : whatTheyPlayed[k])
			{
				memset(numbers, 0, 21);
				for (vector<Card>::iterator i = v.begin(); i != v.end(); i++)
				{
					int j = card2level(*i) + 3;
					if (j == 14)j = 1;
					else if (j == 15)j = 2;
					else if (j == 16)j = 0;
					else if (j == 17)j = -1;
					numbers[++numbers[0]] = j;
				}
				copy(numbers + 1, numbers + numbers[0] + 1, std::ostream_iterator<int>(std::cout, " "));
				if (v.size() == 0)std::cout << "PASS";
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}

		std::cout << "lastValidCombo: ";
		memset(numbers, 0, 21);
		for (vector<Card>::iterator i = lastValidCombo.cards.begin(); i != lastValidCombo.cards.end(); i++)
		{
			int j = card2level(*i) + 3;
			if (j == 14)j = 1;
			else if (j == 15)j = 2;
			else if (j == 16)j = 0;
			else if (j == 17)j = -1;
			numbers[++numbers[0]] = j;
		}
		copy(numbers + 1, numbers + numbers[0] + 1, std::ostream_iterator<int>(std::cout, " "));
		std::cout << std::endl << std::endl;

		std::cout << "cardRemaining: ";
		copy(cardRemaining, cardRemaining + 3, std::ostream_iterator<short>(std::cout, " "));
		std::cout << std::endl;

		std::cout << "myPosition: " << myPosition;
		std::cout << std::endl;

		std::cout << "landlordPosition: " << landlordPosition;
		std::cout << std::endl;

		std::cout << "landlordBid: " << landlordBid;
		std::cout << std::endl << std::endl;

		std::cout << "lastValidPlayer: " << lastValidPlayer;
		std::cout << std::endl << std::endl;

		std::cout << "remaining " << remainingCards.size() << " cards:" << std::endl;
		for (Card c : remainingCards)
		{
			int j = card2level(c) + 3;
			if (j == 14)j = 1;
			else if (j == 15)j = 2;
			else if (j == 16)j = 0;
			else if (j == 17)j = -1;
			std::cout << j << " ";
		}
		std::cout << std::endl << std::endl;*/
#pragma endregion

		BotzoneIO::play(myAction.cards.begin(), myAction.cards.end());
		//std::cout << "running time: " << clock() - startTime << std::endl;
	}
	return 0;
}