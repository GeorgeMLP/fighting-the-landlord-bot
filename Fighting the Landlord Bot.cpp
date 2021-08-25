#include <iostream>
#include <set>
#include <string>
#include <cassert>
#include <cstring> // ע��memset��cstring���
#include <algorithm>
#include "jsoncpp/json.h" // ��ƽ̨�ϣ�C++����ʱĬ�ϰ����˿�
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

#pragma region ��������
enum class Stage
{
	BIDDING, // �зֽ׶�
	PLAYING	 // ���ƽ׶�
};

enum class CardComboType
{
	PASS,		// ��
	SINGLE,		// ����
	PAIR,		// ����
	STRAIGHT,	// ˳��
	STRAIGHT2,	// ˫˳
	TRIPLET,	// ����
	TRIPLET1,	// ����һ
	TRIPLET2,	// ������
	BOMB,		// ը��
	QUADRUPLE2, // �Ĵ�����ֻ��
	QUADRUPLE4, // �Ĵ������ԣ�
	PLANE,		// �ɻ�
	PLANE1,		// �ɻ���С��
	PLANE2,		// �ɻ�������
	SSHUTTLE,	// ����ɻ�
	SSHUTTLE2,	// ����ɻ���С��
	SSHUTTLE4,	// ����ɻ�������
	ROCKET,		// ���
	INVALID		// �Ƿ�����
};

int cardComboScores[] = {
	0,	// ��
	1,	// ����
	2,	// ����
	6,	// ˳��
	6,	// ˫˳
	4,	// ����
	4,	// ����һ
	4,	// ������
	10, // ը��
	8,	// �Ĵ�����ֻ��
	8,	// �Ĵ������ԣ�
	8,	// �ɻ�
	8,	// �ɻ���С��
	8,	// �ɻ�������
	10, // ����ɻ�����Ҫ���У�����Ϊ10�֣�����Ϊ20�֣�
	10, // ����ɻ���С��
	10, // ����ɻ�������
	16, // ���
	0	// �Ƿ�����
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

// ��0~53��54��������ʾΨһ��һ����
using Card = short;
constexpr Card card_joker = 52;
constexpr Card card_JOKER = 53;

// ������0~53��54��������ʾΨһ���ƣ�
// ���ﻹ����һ����ű�ʾ�ƵĴ�С�����ܻ�ɫ�����Ա�Ƚϣ������ȼ���Level��
// ��Ӧ��ϵ���£�
// 3 4 5 6 7 8 9 10	J Q K	A	2	С��	����
// 0 1 2 3 4 5 6 7	8 9 10	11	12	13	14
using Level = short;
constexpr Level MAX_LEVEL = 15;
constexpr Level MAX_STRAIGHT_LEVEL = 11;
constexpr Level level_joker = 13;
constexpr Level level_JOKER = 14;

/**
* ��Card��ȡֵ��Χ0~53�����Level��ȡֵ��Χ0~14��
*/
constexpr Level card2level(Card card)
{
	return card / 4 + card / 53;
}
#pragma endregion

#pragma region ������ʾ�������ƵĽṹ�壬�кܶ����õĳ�Ա�������������ע��
/**
* ���ṹ��������ʾ�������˿��Ƶ����
* ��Ա�ṹ��CardPack������¼ÿ���Ƴ��ֵĴ�����ÿ��CardPack����һ���ƣ���vector<CardPack>��������ʱ�������Ƶ������Ӵ�С�������������Level����
* ��Ա����findMaxSeq������������CardPack��ͷ��ʼ�ݼ��˼��������磬888877775555999444663��Щ�ƣ�����������8888��ʼ���ݼ���7777����5555��7777���������ʺ�������2��
*	�ٱ��磬999888777554463��Щ�ƣ�����������999��ʼ���ݼ���888���ٵݼ���777����55��777������ͬ���ʺ�������3��
* ��Ա����getWeight���ر��ṹ��������Щ�Ƶ�Ȩ�أ��������͵�Ȩ�ؼ��������壩
* ��һ�����캯������һ���յ�CardCombo������ΪPASS��ûʲô��˵�ģ�ע������ͨ��ö����comboType��¼��
* �ڶ������캯����[begin,end)�����ڵ�����Ϊһ��CardCombo��ͬʱ�жϳ���Щ����ʲô���ͣ��絥�š�˳�ӡ�����һ�����Ϸ����ȵȣ�����comboType��¼
* ���������캯����ڶ������ƣ�����������һ��_comboType����[begin,end)�е��Ƶ����ͣ������ڹ��캯���оͲ����ж������ˣ�Ч�ʸ���
* ��Ա����canBeBeatenBy����һ��CardCombo���ж��Լ������ܷ����������
* ��Ա����findFirstValid������[begin,end)���ҳ��ȵ�ǰ���飨�����ṹ������¼�����飩��������飬���統ǰ����Ϊ3334��������ϣ����[begin,end)���ҵ���������飬��5553��6668�ȣ�
*	����������߼��������������Ҵ��ƣ����ҵ������ң����統ǰ�����������333��������444������555���Դ����ƣ���ֻҪ�������ˣ���ѡ�У�Ҳ�����������ƻ᲻���˳�ӻ�ը��
*	�����磬��ǰ������3��[begin,end)�е�����44445�������������ֱ�ӷ���4����ը�����������˻��кܴ������ռ�
* ��Ա����debugPrint���������������Ϣ��������debug
* ��Ա����printCards��cards��������cards�е�������
*/
struct CardCombo
{
	// ��ʾͬ�ȼ������ж�����
	// �ᰴ�����Ӵ�С���ȼ��Ӵ�С����
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
	vector<Card> cards;		 // ԭʼ���ƣ�δ����
	vector<CardPack> packs;	 // ����Ŀ�ʹ�С��������֣�CardPack�ṹ�����һ���ƣ�ͬʱ��¼�������Ƶ�������
	CardComboType comboType; // ���������
	Level comboLevel = 0;	 // ����Ĵ�С�����������Ƶ�Level��

	/**
	* ����������CardPack�ݼ��˼���
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
	* �������������ֵܷ�ʱ���Ȩ��
	*/
	int getWeight() const
	{
		if (comboType == CardComboType::SSHUTTLE ||
			comboType == CardComboType::SSHUTTLE2 ||
			comboType == CardComboType::SSHUTTLE4)
			return cardComboScores[(int)comboType] + (findMaxSeq() > 2) * 10;
		return cardComboScores[(int)comboType];
	}

	// ����һ��������
	CardCombo() : comboType(CardComboType::PASS) {}

	/**
	* ͨ��Card����short�����͵ĵ���������һ������
	* ����������ͺʹ�С���
	* ��������û���ظ����֣����ظ���Card��
	*/
	template <typename CARD_ITERATOR>
	CardCombo(CARD_ITERATOR begin, CARD_ITERATOR end)
	{
		// ���У���
		if (begin == end)
		{
			comboType = CardComboType::PASS;
			return;
		}

		// ÿ�����ж��ٸ�
		short counts[MAX_LEVEL + 1] = {};

		// ͬ���Ƶ��������ж��ٸ����š����ӡ�������������
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

		// ���������������ǿ��ԱȽϴ�С��
		comboLevel = packs[0].level;

		// ��������
		// ���� ͬ���Ƶ����� �м��� ���з���
		vector<int> kindOfCountOfCount;
		for (int i = 0; i <= 4; i++)
			if (countOfCount[i])
				kindOfCountOfCount.push_back(i);
		sort(kindOfCountOfCount.begin(), kindOfCountOfCount.end());

		int curr, lesser;

		switch (kindOfCountOfCount.size())
		{
		case 1: // ֻ��һ����
			curr = countOfCount[kindOfCountOfCount[0]];
			switch (kindOfCountOfCount[0])
			{
			case 1:
				// ֻ�����ɵ���
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
				// ֻ�����ɶ���
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
				// ֻ����������
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
				// ֻ����������
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
		case 2: // ��������
			curr = countOfCount[kindOfCountOfCount[1]];
			lesser = countOfCount[kindOfCountOfCount[0]];
			if (kindOfCountOfCount[1] == 3)
			{
				// ��������
				if (kindOfCountOfCount[0] == 1)
				{
					// ����һ
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
					// ������
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
				// ��������
				if (kindOfCountOfCount[0] == 1)
				{
					// ��������ֻ * n
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
					// ���������� * n
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
	* ����һ�����캯�����ƣ�����������Ӧ��_comboType
	* �����ڹ��캯���оͲ�����ȥ�ж������ˣ�Ч�ʸ���
	* ʹ�øù��캯���豣֤���ṩ��_comboType����ȷ�ģ�����[begin,end)�е��Ƶ�����һ�£�
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
		// ����Ͳ����ж�comboType��
	}

	/**
	* �ж�ָ�������ܷ�����ǰ���飨������������ǹ��Ƶ��������
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
	* ��ָ��������Ѱ�ҵ�һ���ܴ����ǰ���������
	* ��������Ļ�ֻ����һ��
	* ����������򷵻�һ��PASS������
	*/
	template <typename CARD_ITERATOR>
	CardCombo findFirstValid(CARD_ITERATOR begin, CARD_ITERATOR end) const
	{
		if (comboType == CardComboType::PASS) // �������Ҫ���˭��ֻ��Ҫ����
		{
			CARD_ITERATOR second = begin;
			second++;
			return CardCombo(begin, second); // ��ô�ͳ���һ���ơ���
		}

		// Ȼ���ȿ�һ���ǲ��ǻ�����ǵĻ��͹�
		if (comboType == CardComboType::ROCKET)
			return CardCombo();

		// ���ڴ���������дճ�ͬ���͵���
		auto deck = vector<Card>(begin, end); // ����
		short counts[MAX_LEVEL + 1] = {};

		unsigned short kindCount = 0;

		// ����һ��������ÿ�����ж��ٸ�
		for (Card c : deck)
			counts[card2level(c)]++;

		// ������������ã�ֱ�Ӳ��ô��ˣ������ܲ���ը��
		if (deck.size() < cards.size())
			goto failure;

		// ����һ���������ж�������
		for (short c : counts)
			if (c)
				kindCount++;

		// ���򲻶�����ǰ��������ƣ������ܲ����ҵ�ƥ�������
		{
			// ��ʼ��������
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
			for (Level i = 1;; i++) // �������
			{
				for (int j = 0; j < mainPackCount; j++) // �Ҹ��������
				{
					int level = packs[j].level + i;

					// �����������͵����Ʋ��ܵ�2�����������͵����Ʋ��ܵ�С�������ŵ����Ʋ��ܳ�������
					if ((comboType == CardComboType::SINGLE && level > MAX_LEVEL) ||
						(isSequential && level > MAX_STRAIGHT_LEVEL) ||
						(comboType != CardComboType::SINGLE && !isSequential && level >= level_joker))
						goto failure;

					// ��������������Ʋ������Ͳ��ü�������
					if (counts[level] < packs[j].count)
						goto next;
				}
				// ��������������[begin,end)���ҵ��˸�������ƣ��ٸ����ӣ�����ǰ���������Ϊ333444�����������ҵ���444555��555666�ȣ�
				// ����������Ҫ�Ҵ��ƣ��ٸ����ӣ�����ǰ������3334���������Ѿ��ҵ���888��������ֻ������һ�����ƣ��ճ�8883��8884֮��

				{
					// �ҵ��˺��ʵ����ƣ���ô�����أ�
					// ������Ƶ��������������Ǵ��Ƶ��������Ͳ�����Ҳ����
					if (kindCount < packs.size())
						continue;

					// �����ڿ�����
					// ����ÿ���Ƶ�Ҫ����Ŀ��
					short requiredCounts[MAX_LEVEL + 1] = {};
					for (int j = 0; j < mainPackCount; j++)
						requiredCounts[packs[j].level + i] = packs[j].count;
					// ���磬��ǰ�����������333444�������ҵ���������555666����ôrequiredCounts[2]=3��requiredCounts[3]=3������2��5��Level��3��6��Level�����������壩
					for (unsigned j = mainPackCount; j < packs.size(); j++)
					{
						Level k;
						for (k = 0; k <= MAX_LEVEL; k++)
						{
							if (requiredCounts[k] || counts[k] < packs[j].count) // �����packs[j]�������һ�ִ��ƣ����ƿ����ж��֣������Ĵ������������ִ��ƣ�
								continue;
							requiredCounts[k] = packs[j].count; // �ҵ���packs[j]��Ӧ�Ĵ���
							break;
						}
						if (k == MAX_LEVEL + 1) // ����Ƕ�������Ҫ�󡭡��Ͳ����ˣ��Ҳ������ƣ�
							goto next;
					}

					// ��ʼ������
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

			next:; // ������
			}
		}

	failure:
		// ʵ���Ҳ�����
		// ���һ���ܲ���ը��

		for (Level i = 0; i < level_joker; i++)
			if (counts[i] == 4 && (comboType != CardComboType::BOMB || i > packs[0].level)) // ����Է���ը������ը�Ĺ�����
			{
				// ������԰�����
				Card bomb[] = { Card(i * 4), Card(i * 4 + 1), Card(i * 4 + 2), Card(i * 4 + 3) };
				return CardCombo(bomb, bomb + 4);
			}

		// ��û�л����
		if (counts[level_joker] + counts[level_JOKER] == 2)
		{
			Card rocket[] = { card_joker, card_JOKER };
			return CardCombo(rocket, rocket + 2);
		}

		// ����
		return CardCombo();
	}

	/**
	* ����CardCombo����������ζ�ÿ��CardPack��������
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
		std::cout << "��" << cardComboStrings[(int)comboType] << "��" << cards.size() << "�ţ���С��" << comboLevel << "��";
#endif
	}

	/**
	* �����ǰ����������ƣ�0����С����-1�������
	* �Ὣcards����
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

#pragma region ������ʾһ�����ƵĽṹ�壬���Լ�¼���������кϷ�������
/**
* ���ṹ��������ʾһ�����ƣ�ͬʱ���Լ�¼�������еĸ��ֺϷ�����
* ��Ա�ṹ��CardPack������ʾһ���ƣ���3��2��С���ȣ���ͬʱ��¼�������ж������Լ����ǵ�ԭʼ���
* ���캯����[begin,end)�е������Ƽ�¼��packs��
* ��Ա����printCards���⸱���������0����С����-1�������
* ��Ա����getAllSingle~getAllRocket����ͳ���⸱�������кϷ���ĳ�����ͣ�����¼����Ӧ��vector<CardCombo>��
*	���磬3344556789�⸱�ƣ�����getAllStraight2()����˫˳334455��һ��CardCombo�ṹ�����ʽ����¼��straight2��
* ��Ա����getAllCombos��getAllSingle~getAllRocketȫ��ִ��һ�飬��¼���������кϷ�����
* ��Ա����printAllCombos��ִ��getAllCombos()���ٽ����������кϷ��������
* ��Ա����erase��[begin,end)���������Ƴ����ƣ��豣֤����������Щ��
* ��Ա����insert��[begin,end)�������Ƽ������ƣ��豣֤������û����Щ��
* ��Ա����getLength���������й��ж�������
*/
struct CardCombinations
{
	// ��ʾͬ�ȼ������ж�����
	struct CardPack
	{
		Level level;
		short count = 0;
		Card card[4];
	};
	CardPack packs[MAX_LEVEL];	 // ����һ���ƣ�ͬʱ��¼�������Ƶ��������Ƶ�ԭʼ���

	// ע�⣺�������Щvector��¼�����Ƶ�Level�������Ƶ�ԭʼ���
	vector<CardCombo> single;
	vector<CardCombo> pair;
	vector<CardCombo> straight[13];	//straight[i]��ʾ��i���Ƶ�˳�ӣ�����ͬ��
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
	* Ĭ�Ϲ��캯��
	*/
	CardCombinations() {}

	/**
	* ���캯���������ǽ�[begin,end)�е������Ƽ�¼��packs��
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
	* �����ǰ���Ƶ������ƣ�0����С����-1�������
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
	* ���º������ڼ�¼�ø������еĸ��ֺϷ����ͣ���ͬ���ͽ�����һ��
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
		for (int i = 0; i <= 12; i++)  // ����
		{
			if (packs[i].count < 3)continue;
			for (int j = 0; j <= 14; j++)  // ����
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
		for (int i = 0; i <= 12; i++)  // ����
		{
			if (packs[i].count < 3)continue;
			for (int j = 0; j <= 12; j++)  // ����
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
		for (int i = 0; i <= 12; i++)  // ����
		{
			if (packs[i].count != 4)continue;
			for (int j = 0; j <= 14; j++)  // ����1
			{
				if (j == i || packs[j].count == 0)continue;
				for (int k = j + 1; k <= 14; k++)  // ����2
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
		for (int i = 0; i <= 12; i++)  // ����
		{
			if (packs[i].count < 4)continue;
			for (int j = 0; j <= 12; j++)  // ����1
			{
				if (j == i || packs[j].count < 2)continue;
				for (int k = j + 1; k <= 12; k++)  // ����2
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
				int w = 2 * (k - i + 1);  // ��Ҫ�Ĵ�������
				if (w > 13 - k + i)break;  // ��������������
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
				int w = 2 * (k - i + 1);  // ��Ҫ�Ĵ��ƶ���
				if (w > 11 - k + i)break;  // ��������������
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
	* �����ǰ���Ƶ����кϷ�����
	* ���ø���x
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
	* ��[begin,end)�����е��ƴӵ�ǰ�������Ƴ�
	* ��ȷ����ǰ��������[begin,end)�е�������
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
	* ��[begin,end)�����е��Ƽ��뵱ǰ����
	* ��ȷ����ǰ������û��[begin,end)�е��κ���
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
	* �������ƹ��ж�������
	*/
	constexpr int getLength()
	{
		int length = 0;
		for (int i = 0; i <= 14; i++)length += packs[i].count;
		return length;
	}
};
#pragma endregion

#pragma region ��ȡ���룬�ָ�״̬��������д��JSON�ļ������
/**
* ��botzone�Ĺ���ÿ���غϣ�bot�������������ģ�Ҳ����˵�������޷���¼֮ǰ�ķ��ƺͳ�����ʷ��
* ��ˣ�������Ҫ�������JSON�ļ��ж�ȡ��ʷ��������ָ�����ǰ���棬�ٽ��о��ߣ�
* ���ϴ�botzone��ʱ�򻹿���ѡ��ʱģʽ�����غϽ�����bot����ֹͣ���У���������˵���ģʽ������������׳��������Ȱ���ͨģʽʵ�֣���ȥ���Գ�ʱģʽ��
* ����Ĵ������������ȡ���롢�ָ�״̬��������ߵģ�������������������������������ģ������play���������ǰѾ���д��JSON�ļ�����ʽ�����
*/

// �ҵ�������Щ
set<Card> myCards;

// ������ʾ��������Щ
set<Card> landlordPublicCards;

// ��Ҵ��ʼ�����ڶ�����ʲô
vector<vector<Card>> whatTheyPlayed[PLAYER_COUNT];

// ��ǰҪ��������Ҫ���˭
CardCombo lastValidCombo;

// lastValidCombo�Ǽ�����ҳ���
int lastValidPlayer;

// ��һ�ʣ������
short cardRemaining[PLAYER_COUNT] = { 17, 17, 17 };

// ���Ǽ������
int myPosition;

// ����λ��
int landlordPosition = -1;

// �����з�
int landlordBid = -1;

// �׶�
Stage stage = Stage::BIDDING;

// �Լ��ĵ�һ�غ��յ��Ľз־���
vector<int> bidInput;

// ����ʼ���е�ʱ�䣬���ڿ�ʱ
int startTime;

// ���ڲ������ؿ����ڱ��غ�botzone����ģ�⼸��
int gamesSimulated = 0;

// ���ڲ�����Botzone�����ؿ���÷���ߺ͵ڶ��ߵľ��������ٷ�
int differenceBetweenFirstAndSecond = 0;

namespace BotzoneIO
{
	using namespace std;
	void read()
	{
		// �������루ƽ̨�ϵ������ǵ��У�
		string line;
		getline(cin, line);
		Json::Value input;
		Json::Reader reader;
		reader.parse(line, input);
		startTime = clock();

		// ���ȴ����һ�غϣ���֪�Լ���˭������Щ��
		{
			auto firstRequest = input["requests"][0u]; // �±���Ҫ�� unsigned������ͨ�������ֺ����u������
			auto own = firstRequest["own"];
			for (unsigned i = 0; i < own.size(); i++)
				myCards.insert(own[i].asInt());
			if (!firstRequest["bid"].isNull())
			{
				// ��������Խз֣����¼�з�
				auto bidHistory = firstRequest["bid"];
				myPosition = bidHistory.size();
				for (unsigned i = 0; i < bidHistory.size(); i++)
					bidInput.push_back(bidHistory[i].asInt());
			}
		}

		// history���һ����ϼң��͵ڶ���ϼң��ֱ���˭�ľ���
		int whoInHistory[2] = {};

		int turn = input["requests"].size();
		for (int i = 0; i < turn; i++)
		{
			auto request = input["requests"][i];
			auto llpublic = request["publiccard"];
			if (!llpublic.isNull())
			{
				// ��һ�ε�֪�����ơ������зֺ͵�����˭
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

			auto history = request["history"]; // ÿ����ʷ�����ϼҺ����ϼҳ�����
			if (history.isNull())
				continue;
			stage = Stage::PLAYING;

			// ��λָ����浽��ǰ
			int howManyPass = 0;
			for (int p = 0; p < 2; p++)
			{
				int player = whoInHistory[p];	// ��˭������
				auto playerAction = history[p]; // ������Щ��
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // ѭ��ö������˳���������
				{
					int card = playerAction[_].asInt(); // �����ǳ���һ����
					playedCards.push_back(card);
				}
				whatTheyPlayed[player].push_back(playedCards); // ��¼�����ʷ
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
				// ��Ҫ�ָ��Լ�������������
				auto playerAction = input["responses"][i]; // ������Щ��
				vector<Card> playedCards;
				for (unsigned _ = 0; _ < playerAction.size(); _++) // ѭ��ö���Լ�����������
				{
					int card = playerAction[_].asInt(); // �������Լ�����һ����
					myCards.erase(card);				// ���Լ�������ɾ��
					playedCards.push_back(card);
				}
				whatTheyPlayed[myPosition].push_back(playedCards); // ��¼�����ʷ
				cardRemaining[myPosition] -= playerAction.size();
			}
		}
	}

	/**
	* ����з֣�0, 1, 2, 3 ����֮һ��
	*/
	void bid(int value)
	{
		Json::Value result;
		result["response"] = value;

		Json::FastWriter writer;
		cout << writer.write(result) << endl;
	}

	/**
	* ������ƾ��ߣ�begin�ǵ�������㣬end�ǵ������յ�
	* CARD_ITERATOR��Card����short�����͵ĵ�����
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

#pragma region ���õĺ���
/**
* ����һ�����ƺ���Ҫ��������ͣ��������������б�lastValidCombo������ͣ���������ը��ը���������
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
	// �Ȱ�ը������ը�ӵ����еĲ�����
	hand.getAllRocket();
	for (CardCombo rocket : hand.rocket)
		actions.push_back(rocket);
	hand.getAllBomb();
	for (CardCombo bomb : hand.bomb)
		actions.push_back(bomb);
	// �ٷ�������
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
* ���������CardCombo��ɵ�vector������CardCombo������
* ������һ��������debug
*/
void printActions(vector<CardCombo>& actions)
{
	for (CardCombo action : actions)
		action.printCards();
}
#pragma endregion

#pragma region һ���Ľ���ֵ�����Ͳ��Ե�AI
int getUtility(CardCombinations& hand, int type, const int alpha);

/**
* ����һ�����ƣ������⸱�ƵĹ�ֵutility(Hand, alpha), alphaΪ�ɱ����
* ��ֵ������˼·�ǣ������Ѱ����Ʋ��������ͣ�ÿһ�ֲ𷨶�Ӧһ��utility�����շ��������ֵ��Ҳ���Ƕ�Ӧ����Ѳ�
* ��ֵ����Խ����ζ��(1)���Ƶ�ѹ����Խ��(2)����Խ���ܳ��ꡣ�ڲ�ͬ��ʱ��(1)��(2)��Ȩ���ǲ�ͬ�ġ�
* ��alpha������Ȩ�ء���Լ�����յĹ�ֵ = ���Ƶ����������� - alpha * �����������
* alphaԽС��Խ���Ӵ���������alphaԽ��Խ�����ٶ�
* ����ʱ���ǵ�˳��Ϊ�������ը�����ɻ������ԡ�˳�ӡ����������ӡ����š�ʲô�Ĵ���������ɻ���û��˵����www
*/
int utility(CardCombinations& hand, const int alpha = 10)
{
	return getUtility(hand, 0, alpha);
}

int getUtility(CardCombinations& hand, int type, const int alpha)
{
	int utility1 = 0, utility2 = 0;	//�ֱ��Ӧ������ֱ�Ӵ���Ͳ���������ֲ���
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
		if (!hand.bomb.size())	//û��ը����ֱ���ж���һ������
			return getUtility(hand, 2, alpha);
		else
		{
			CardCombo biggestBomb = *hand.bomb.begin();
			hand.erase(biggestBomb.cards.begin(), biggestBomb.cards.end());	//ɾ������ը��֮��ݹ�
			utility1 = getUtility(hand, 1, alpha) + biggestBomb.comboLevel;	//ը���Ƚ����⣬���ڲ������������������Լ�����һ���ƣ�������+alpha����-alpha
			hand.insert(biggestBomb.cards.begin(), biggestBomb.cards.end());
			utility2 = getUtility(hand, 2, alpha);	//��ȻҲ����ѡ���ը���𿪳�
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
		hand.getAllSingle();	//Ϊ�˼ӿ��ٶ���������š����ӡ�����һ����
		for (auto singlei = hand.single.begin(); singlei != hand.single.end(); singlei++)
		{
			utility1 += singlei->comboLevel + hand.packs[singlei->comboLevel].count - alpha;	//���űȵ�������ѹ�һЩ�����Թ�ֵ����
			if (singlei->comboLevel >= 11)	//A, 2, С��, �����Ƚ�ǿ���ж���ӳ�
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
 * ����������������ը���ͻ��������������
 * ����˵���������ը����ʱ���Լ�����Ӧ�ý���һЩ�������Գ����ƺ�ը����
 * ������զд�һ�û��ã��������������ʱû���ھ��ߺ����������
 */
int howManyBombsOutside()
{
	CardCombinations outsideCards(remainingCards.begin(), remainingCards.end());
	outsideCards.getAllBomb();
	outsideCards.getAllRocket();
	return outsideCards.bomb.size() + outsideCards.rocket.size();
}

/**
* �������ƺͱ��γ���Ҫ������ƣ��Լ��ɱ����alpha, beta������Ҫ������
* ����˼·�ǣ����������ܳ��ĳ��Ʒ�ʽ��ѡ��ʹ(10 * utility����ֵ + beta * ����)����һ�֡�
* ��Ȼ��betaԽ��Խ������ѹ�ơ���beta�㹻Сʱai��ѡ��pass�������Լ�����������Ϊǿ��ѹ�ƿ��ܻ���utility��С����pass����
* �ر�أ�����Լ��ǵ����¼��ҵ����ϼұ������Լ�����ʱ�����С�ĵ��ơ�
*/
CardCombo _findBestAction(CardCombinations& hand, const CardCombo& lastValidCombo, int alpha = 19, const int beta = 6)
{
	vector<CardCombo> actions = getActions(hand, lastValidCombo);
	if (actions.size() == 0)return CardCombo();
	CardCombo bestAction = CardCombo();
	int bestMeasure = lastValidCombo.cards.size() ? 0 : -233333;	//����Լ����������������������
	if (!lastValidCombo.cards.size())	//����Լ������Ļ�����������ٶ�
		alpha += 3;
	int originalUtility = utility(hand, alpha);
	int tempMeasure;
	for (CardCombo action : actions)
	{
		hand.erase(action.cards.begin(), action.cards.end());
		if (!hand.getLength())	//������������ʹ���ĸ�2������ҲҪȫ˦��ȥx��
		{
			hand.insert(action.cards.begin(), action.cards.end());
			return action;
		}
		tempMeasure = 10 * (utility(hand, alpha) - originalUtility) + beta * action.comboLevel;
		if (action.comboType == CardComboType::BOMB || action.comboType == CardComboType::ROCKET)
			tempMeasure += beta * 6;	//�����ֵ��ʾ��ը��ƫ�ó̶ȣ�ը��Ŀ����alpha: ��������� beta:������
		else if (action.comboLevel >= 11)	//A, 2, С��, ������ǿ���мӳ�
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
	if (position == (landlordPosition + 2) % 3 && lastValidPlayer == landlordPosition)	//������֪�����ϼ�Ҫѹ��
		beta += 5;
	if (position == (landlordPosition + 2) % 3 && lastValidPlayer == (landlordPosition + 1) % 3 && lastValidCombo.comboLevel >= 9)	//�����¼Ҵ��˴��ƣ��ǵ����ϼҲ�Ҫ
		beta -= lastValidCombo.comboLevel - 8;
	if (position == (landlordPosition + 1) % 3 && lastValidPlayer == (landlordPosition + 2) % 3)	//�����ϼҴ��Ƶ�����Ҫ���ǵ����¼�Ҳ��Ҫ
		beta -= 5;
	return _findBestAction(hand, lastValidCombo, alpha, beta);
}

/**
* ������õ�n�����ߣ���ǰ���е�action����n���Ļ��ͷ������п��е�actions����ԭ������һ������һ��
*/
vector<CardCombo> _findBestNActions(int _n, CardCombinations& hand, const CardCombo& lastValidCombo, int alpha = 19, const int beta = 6)
{
	vector<CardCombo> actions = getActions(hand, lastValidCombo);
	int n = std::min(_n, int(actions.size()));
	vector<CardCombo> bestActions;
	std::multimap<int, CardCombo> actionsScores;
	if (!lastValidCombo.cards.size())	//����Լ������Ļ�����������ٶ�
		alpha += 3;
	int originalUtility = utility(hand, alpha);
	int tempMeasure;
	for (CardCombo action : actions)
	{
		hand.erase(action.cards.begin(), action.cards.end());
		tempMeasure = 10 * (utility(hand, alpha) - originalUtility) + beta * action.comboLevel;
		if (action.comboType == CardComboType::BOMB || action.comboType == CardComboType::ROCKET)
			tempMeasure += beta * 6;	//�����ֵ��ʾ��ը��ƫ�ó̶ȣ�ը��Ŀ����alpha: ��������� beta:������
		else if (action.comboLevel >= 11)	//A, 2, С��, ������ǿ���мӳ�
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
 * �ֶ�����һЩ�߼��ж�
 */
vector<CardCombo> findBestNActions(int _n, CardCombinations& hand, const CardCombo& lastValidCombo, int position, int landlordPosition)
{
	int alpha = 19, beta = 6;
	int enemyLeft = 0;
	if (position == landlordPosition)
		enemyLeft = std::min(cardRemaining[(position + 1) % 3], cardRemaining[(position + 2) % 3]);
	else
		enemyLeft = cardRemaining[landlordPosition];
	if (enemyLeft <= 5)	//���ֿ�Ҫ�����ˣ�
		beta += 6 - enemyLeft;
	if (position == (landlordPosition + 2) % 3 && lastValidPlayer == landlordPosition)	//������֪�����ϼ�Ҫѹ��
		beta += 5;
	if (position == (landlordPosition + 2) % 3 && lastValidPlayer == (landlordPosition + 1) % 3 && lastValidCombo.comboLevel >= 9)	//�����¼Ҵ��˴��ƣ��ǵ����ϼҲ�Ҫ
		beta -= lastValidCombo.comboLevel - 8;
	if (position == (landlordPosition + 1) % 3 && lastValidPlayer == (landlordPosition + 2) % 3)	//�����ϼҴ��Ƶ�����Ҫ���ǵ����¼�Ҳ��Ҫ
		beta -= 5;
	return _findBestNActions(_n, hand, lastValidCombo, alpha, beta);
}
#pragma endregion

#pragma region ���ؿ���������
//set<Card> remainingCards;	ǰ���и�����Ҫ�ã�����ǰ�涨����
CardCombinations myHand;  // �ҵ�����
CardCombinations nextHand;  // �¼ҵ�����
CardCombinations previousHand;  // �ϼҵ�����
vector<CardCombo> rootActions;  // �ҵĿ��еľ��ߣ���findBestNActions()���ص�n�����ߣ�
std::map<CardCombo, int> winningTimes;  // ����ÿ�����ߵ�totalScore֮��
int totalScore = 1;  // ÿ��ģ��Ծֵ����յ÷֣�Ĭ��Ϊ1������ը������������졢����������򷭱�
bool landlordHasNotPlayed = true;  // ����������Ϊֹ�����˵�һ���ƣ���û�г�����
bool landlordHasNotPlayed_;  // ��ÿ��ģ��Ծ��м�¼�������˵�һ������û�г�����

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
}root;  // ���ؿ������ڵ�
std::multimap<CardCombo, TreeNode*> sons;  // �ҵľ��ߺͶ�Ӧ�Ľڵ�һһ��Ӧ���ɵ�map

/**
* ׼�����ؿ���ģ����Ҫ�����ݣ���myHand��remainingCards��root.hands[myPosition]��root.turn��rootActions��¼����������ʼ��winningTimes
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
* ��remainingCards������ҷָ�����������ң��������ݼ�¼��root.hands��sons��
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
* ����ģ��Ծ֣����ر��ζԾֵ�totalScore
* ģ��Ծֵ��߼��ǣ��ڵ�ǰ��ҳ���֮���ģ���У�ÿ����Ҷ�����findBestAction()���صľ��߳���
* �׷�Ϊ1������ը������������졢������������ͷַ�������Ȼÿ�ֶԾֿ�ʼʱ�׷�totalScoreҪ�ָ�Ϊ1��
* �������ǰ��һ�ʤ���������յ÷�totalScore�����򣬷���-toalScore
* �Ե�ǰ�����õ�3�����߽��ж��ģ�⣬����ѡ��totalScore֮�����ľ�����Ϊ���ƾ���
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
* ��timeLimitʱ�䷶Χ�ڽ��о����ܶ��ģ��Ծ֣���ÿ�־��ߵ�totalScore֮�������
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
* ����totalScore֮�����ľ���
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
	prepareData();  // ��¼myHand��remainingCards

	if (stage == Stage::BIDDING)
	{
#pragma region ���Զ����ƵĹ�ֵ
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
		// �������ߣ���ֻ���޸����²��֣�

		int bidValue = utility(myHand, 10) >= 0 ? 3 : 0;	//����������ɣ�����ֻҪ�е�������3�֣�x
		//int bidValue = 3;
		// ���߽���������������ֻ���޸����ϲ��֣�

		BotzoneIO::bid(bidValue);
	}
	else if (stage == Stage::PLAYING)
	{
		// �������ߣ���ֻ���޸����²��֣�;

		CardCombo myAction = !whatTheyPlayed[myPosition].size() ? findBestAction(myHand, lastValidCombo, myPosition, landlordPosition)
			: returnAction();
		if (!myAction.cards.size())myAction = *rootActions.begin();

#pragma region ���Գ������
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
#pragma region �����ǰ״̬��ر���
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