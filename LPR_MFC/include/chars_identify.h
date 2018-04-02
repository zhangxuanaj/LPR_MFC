
#ifndef __CHARS_IDENTIFY_H__
#define __CHARS_IDENTIFY_H__

#include "prep.h"
#include "character.hpp"
#include <memory>
//#include "../include/kv.h"


namespace lpr {

	

	static const int kCharactersNumber = 34;
	static const int kChineseNumber = 31;
	static const int kCharsTotalNumber = 65;
	static const int kCharacterSize = 10;
	static const int kChineseSize = 20;
	static const int kPredictSize = kCharacterSize;

	static const char* kDefaultAnnPath = "model/ann.xml";
	static const char* kChineseAnnPath = "model/ann_chinese.xml";

	static const char *kChars[] = 
	{
		"0", "1", "2",
		"3", "4", "5",
		"6", "7", "8",
		"9",
		/*  10  */
		"A", "B", "C",
		"D", "E", "F",
		"G", "H", /* {"I", "I"} */
		"J", "K", "L",
		"M", "N", /* {"O", "O"} */
		"P", "Q", "R",
		"S", "T", "U",
		"V", "W", "X",
		"Y", "Z",
		/*  24  */
		"zh_chuan", "zh_e", "zh_gan",
		"zh_gan1", "zh_gui", "zh_gui1",
		"zh_hei", "zh_hu", "zh_ji",
		"zh_jin", "zh_jing", "zh_jl",
		"zh_liao", "zh_lu", "zh_meng",
		"zh_min", "zh_ning", "zh_qing",
		"zh_qiong", "zh_shan", "zh_su",
		"zh_jin", "zh_wan", "zh_xiang",
		"zh_xin", "zh_yu", "zh_yu1",
		"zh_yue", "zh_yun", "zh_zang",
		"zh_zhe"
		/*  31  */
	};


	class CharsIdentify 
	{
	public:
		static CharsIdentify* instance();

		int classify(cv::Mat f, float& maxVal, bool isChinses = false);
		void classify(cv::Mat featureRows, std::vector<int>& out_maxIndexs,
			std::vector<float>& out_maxVals, std::vector<bool> isChineseVec);
		void classify(std::vector<CCharacter>& charVec);
		void classifyChinese(std::vector<CCharacter>& charVec);

		std::pair<std::string, std::string> identify(cv::Mat input, bool isChinese = false);
		int identify(std::vector<cv::Mat> inputs, std::vector<std::pair<std::string, std::string>>& outputs,
			std::vector<bool> isChineseVec);

		std::pair<std::string, std::string> identifyChinese(cv::Mat input, float& result, bool& isChinese);

		bool isCharacter(cv::Mat input, std::string& label, float& maxVal, bool isChinese = false);

		void LoadModel(std::string path);
		void LoadChineseModel(std::string path);

	private:
		CharsIdentify();

		static CharsIdentify* instance_;
		cv::Ptr<cv::ml::ANN_MLP> ann_;
		cv::Ptr<cv::ml::ANN_MLP> annChinese_;
		//std::shared_ptr<Kv> kv_;

		//! 省份对应map
		std::map<string, string> m_map;
	};
} 


#endif 