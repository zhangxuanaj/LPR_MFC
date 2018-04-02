#include "stdafx.h"
#include "../include/chars_identify.h"

#include "../include/features.h"

//#include<memory>

namespace lpr
{


	CharsIdentify* CharsIdentify::instance_ = nullptr;

	CharsIdentify* CharsIdentify::instance() 
	{
		if (!instance_) 
		{
			instance_ = new CharsIdentify;
		}
		return instance_;
	}

	CharsIdentify::CharsIdentify() 
	{
		//映射表
		typedef pair<string, string> CodeProvince;

		if (m_map.empty()) {
			m_map.insert(CodeProvince("zh_chuan", "川"));
			m_map.insert(CodeProvince("zh_e", "鄂"));
			m_map.insert(CodeProvince("zh_gan", "赣"));
			m_map.insert(CodeProvince("zh_gan1", "甘"));
			m_map.insert(CodeProvince("zh_gui", "贵"));
			m_map.insert(CodeProvince("zh_gui1", "桂"));
			m_map.insert(CodeProvince("zh_hei", "黑"));
			m_map.insert(CodeProvince("zh_hu", "沪"));
			m_map.insert(CodeProvince("zh_ji", "冀"));
			m_map.insert(CodeProvince("zh_jin", "津"));
			m_map.insert(CodeProvince("zh_jing", "京"));
			m_map.insert(CodeProvince("zh_jl", "吉"));
			m_map.insert(CodeProvince("zh_liao", "辽"));
			m_map.insert(CodeProvince("zh_lu", "鲁"));
			m_map.insert(CodeProvince("zh_meng", "蒙"));
			m_map.insert(CodeProvince("zh_min", "闽"));
			m_map.insert(CodeProvince("zh_ning", "宁"));
			m_map.insert(CodeProvince("zh_qing", "青"));
			m_map.insert(CodeProvince("zh_qiong", "琼"));
			m_map.insert(CodeProvince("zh_shan", "陕"));
			m_map.insert(CodeProvince("zh_su", "苏"));
			m_map.insert(CodeProvince("zh_jin", "晋"));
			m_map.insert(CodeProvince("zh_wan", "皖"));
			m_map.insert(CodeProvince("zh_xiang", "湘"));
			m_map.insert(CodeProvince("zh_xin", "新"));
			m_map.insert(CodeProvince("zh_yu", "豫"));
			m_map.insert(CodeProvince("zh_yu1", "渝"));
			m_map.insert(CodeProvince("zh_yue", "粤"));
			m_map.insert(CodeProvince("zh_yun", "云"));
			m_map.insert(CodeProvince("zh_zang", "藏"));
			m_map.insert(CodeProvince("zh_zhe", "浙"));
		}
	
		ann_ = ml::ANN_MLP::load<ml::ANN_MLP>(kDefaultAnnPath);
		annChinese_ = ml::ANN_MLP::load<ml::ANN_MLP>(kChineseAnnPath);
	
	}

	void CharsIdentify::LoadModel(std::string path) {
		if (path != std::string(kDefaultAnnPath)) {

			if (!ann_->empty())
				ann_->clear();

			ann_ = ml::ANN_MLP::load<ml::ANN_MLP>(path);
		}
	}

	void CharsIdentify::LoadChineseModel(std::string path) {
		if (path != std::string(kChineseAnnPath)) {

			if (!annChinese_->empty())
				annChinese_->clear();

			annChinese_ = ml::ANN_MLP::load<ml::ANN_MLP>(path);
		}
	}

	void CharsIdentify::classify(cv::Mat featureRows, std::vector<int>& out_maxIndexs,
		std::vector<float>& out_maxVals, std::vector<bool> isChineseVec){
		int rowNum = featureRows.rows;

		cv::Mat output(rowNum, kCharsTotalNumber, CV_32FC1);
		ann_->predict(featureRows, output);

		for (int output_index = 0; output_index < rowNum; output_index++) {
			Mat output_row = output.row(output_index);
			int result = -1;
			float maxVal = -2.f;
			bool isChinses = isChineseVec[output_index];
			if (!isChinses) {
				result = 0;
				for (int j = 0; j < kCharactersNumber; j++) {
					float val = output_row.at<float>(j);
					// std::cout << "j:" << j << "val:" << val << std::endl;
					if (val > maxVal) {
						maxVal = val;
						result = j;
					}
				}
			}
			else {
				result = kCharactersNumber;
				for (int j = kCharactersNumber; j < kCharsTotalNumber; j++) {
					float val = output_row.at<float>(j);
					//std::cout << "j:" << j << "val:" << val << std::endl;
					if (val > maxVal) {
						maxVal = val;
						result = j;
					}
				}
			}
			out_maxIndexs[output_index] = result;
			out_maxVals[output_index] = maxVal;
		}
	}


	void CharsIdentify::classify(std::vector<CCharacter>& charVec)
	{
		size_t charVecSize = charVec.size();

		if (charVecSize == 0)
			return;

		Mat featureRows;
		for (size_t index = 0; index < charVecSize; index++) 
		{
			Mat charInput = charVec[index].getCharacterMat();
			Mat feature = charFeatures(charInput, kPredictSize); //计算输入的charInput的文字特征，输入charInput低分辨率尺寸为kPredictSize*kPredictSize（10×10），输出是单行矩阵
			featureRows.push_back(feature); //Mat类push_back，参数为mat时（feature），feature列数必须与Mat容器featureRows列数相同
			//featureRows每行是每个字块的文字特征
		}

		cv::Mat output(charVecSize, kCharsTotalNumber, CV_32FC1);
		ann_->predict(featureRows, output);//神经网络方法predict，输入是文字特征mat类（所有charVec中字块的结果，每行是一个字块的文字特征），输出mat类
		//output中应该存着概率值，可用来进行非极大值抑制
		//如果output某行（对应某个字块的预测结果）最大元素值都很小，表明这个字块很可能不是字

		for (size_t output_index = 0; output_index < charVecSize; output_index++) 
		{
			CCharacter& character = charVec[output_index];
			Mat output_row = output.row(output_index); //output每行应该是每个字块的ann检测结果，即每个元素是对每个数字字母文字的响应按概率，概率最大的作为最终字块决策输出

			int result = -1;
			float maxVal = -2.f;//maxVal是概率最大值
			std::string label = "";

			bool isChinses = character.getIsChinese();
			if (!isChinses) 
			{
				result = 0;
				for (int j = 0; j < kCharactersNumber; j++) 
				{
					float val = output_row.at<float>(j);
					//std::cout << "j:" << j << "val:" << val << std::endl;
					if (val > maxVal) {
						maxVal = val;
						result = j;
					}
				}
				label = std::make_pair(kChars[result], kChars[result]).second;
			}
			else 
			{
				result = kCharactersNumber;
				for (int j = kCharactersNumber; j < kCharsTotalNumber; j++)
				{
					float val = output_row.at<float>(j);
					//std::cout << "j:" << j << "val:" << val << std::endl;
					if (val > maxVal) 
					{
						maxVal = val;
						result = j;
					}
				}
				const char* key = kChars[result];
				std::string s = key;
				std::string province = m_map[s];
;				//std::string province = kv_->get(s);
				label = std::make_pair(s, province).second;
			}
			/*std::cout << "result:" << result << std::endl;
			std::cout << "maxVal:" << maxVal << std::endl;*/
			character.setCharacterScore(maxVal);
			character.setCharacterStr(label);
		}
	}


	void CharsIdentify::classifyChinese(std::vector<CCharacter>& charVec)
	{
		size_t charVecSize = charVec.size();

		if (charVecSize == 0)
			return;

		Mat featureRows;
		for (size_t index = 0; index < charVecSize; index++)
		{
			Mat charInput = charVec[index].getCharacterMat();
			Mat feature = charFeatures(charInput, kChineseSize);
			featureRows.push_back(feature);
		}

		cv::Mat output(charVecSize, kChineseNumber, CV_32FC1);
		annChinese_->predict(featureRows, output);

		for (size_t output_index = 0; output_index < charVecSize; output_index++) 
		{
			CCharacter& character = charVec[output_index];
			Mat output_row = output.row(output_index);
			bool isChinese = true;
			
			float maxVal = -2;
			int result = -1;

			for (int j = 0; j < kChineseNumber; j++) 
			{
				float val = output_row.at<float>(j);
				//std::cout << "j:" << j << "val:" << val << std::endl;
				if (val > maxVal) 
				{
					maxVal = val;
					result = j;
				}
			}

			// no match
			if (-1 == result) {
				result = 0;
				maxVal = 0;
				isChinese = false;
			}

			auto index = result + kCharsTotalNumber - kChineseNumber;
			const char* key = kChars[index];
			std::string s = key;
			std::string province = m_map[s];
			//std::string province = kv_->get(s);

			/*std::cout << "result:" << result << std::endl;
			std::cout << "maxVal:" << maxVal << std::endl;*/

			character.setCharacterScore(maxVal);
			character.setCharacterStr(province);
			character.setIsChinese(isChinese);
		}
	}

	int CharsIdentify::classify(cv::Mat f, float& maxVal, bool isChinses){
		int result = -1;

		cv::Mat output(1, kCharsTotalNumber, CV_32FC1);
		ann_->predict(f, output);

		maxVal = -2.f;
		if (!isChinses) {
			result = 0;
			for (int j = 0; j < kCharactersNumber; j++) {
				float val = output.at<float>(j);
				// std::cout << "j:" << j << "val:" << val << std::endl;
				if (val > maxVal) {
					maxVal = val;
					result = j;
				}
			}
		}
		else {
			result = kCharactersNumber;
			for (int j = kCharactersNumber; j < kCharsTotalNumber; j++) {
				float val = output.at<float>(j);
				//std::cout << "j:" << j << "val:" << val << std::endl;
				if (val > maxVal) {
					maxVal = val;
					result = j;
				}
			}
		}
		//std::cout << "maxVal:" << maxVal << std::endl;
		return result;
	}

	//判断字块是否是汉字
	bool CharsIdentify::isCharacter(cv::Mat input, std::string& label, float& maxVal, bool isChinese) 
	{
		cv::Mat feature = charFeatures(input, kPredictSize);
		auto index = static_cast<int>(classify(feature, maxVal, isChinese));

		if (isChinese) 
		{
			//std::cout << "maxVal:" << maxVal << std::endl;
		}


		float chineseMaxThresh = 0.2f;
		//float chineseMaxThresh = CParams::instance()->getParam2f();

		if (maxVal >= 0.9 || (isChinese && maxVal >= chineseMaxThresh)) 
		{
			if (index < kCharactersNumber) 
			{
				label = std::make_pair(kChars[index], kChars[index]).second;
			}
			else 
			{
				const char* key = kChars[index];
				std::string s = key;
				std::string province = m_map[s];
				//std::string province = kv_->get(s);
				label = std::make_pair(s, province).second;
			}
			return true;
		}
		else
			return false;
	}


	std::pair<std::string, std::string> CharsIdentify::identifyChinese(cv::Mat input, float& out, bool& isChinese) 
	{
		cv::Mat feature = charFeatures(input, kChineseSize);
		float maxVal = -2;

		int result = -1;

		cv::Mat output(1, kChineseNumber, CV_32FC1);
		annChinese_->predict(feature, output);
		//cout << output << endl;
		for (int j = 0; j < kChineseNumber; j++) 
		{
			float val = output.at<float>(j);
			//std::cout << "j:" << j << "val:" << val << std::endl;
			if (val > maxVal) 
			{
				maxVal = val;
				result = j;
			}
		}

		// no match
		if (-1 == result) 
		{
			result = 0;
			maxVal = 0;
			isChinese = false;
		}
		else if (maxVal > 0.9)
		{
			isChinese = true;
		}

		auto index = result + kCharsTotalNumber - kChineseNumber;
		const char* key = kChars[index];
		std::string s = key;

		//cout << s << endl;
		std::string province = m_map[s];
		//std::string province = kv_->get(s);

		//cout << province << endl;

		out = maxVal;
		
		return std::make_pair(s, province);
	}


	std::pair<std::string, std::string> CharsIdentify::identify(cv::Mat input, bool isChinese) 
	{
		cv::Mat feature = charFeatures(input, kPredictSize);
		float maxVal = -2;
		auto index = static_cast<int>(classify(feature, maxVal, isChinese));
		if (index < kCharactersNumber) 
		{
			return std::make_pair(kChars[index], kChars[index]);
		}
		else 
		{
			const char* key = kChars[index];
			std::string s = key;
			std::string province = m_map[s];
			//std::string province = kv_->get(s);
			return std::make_pair(s, province);
		}
	}

	int CharsIdentify::identify(std::vector<cv::Mat> inputs, std::vector<std::pair<std::string, std::string>>& outputs,
		std::vector<bool> isChineseVec) 
	{
		Mat featureRows;
		size_t input_size = inputs.size();
		for (size_t i = 0; i < input_size; i++) 
		{
			Mat input = inputs[i];
			cv::Mat feature = charFeatures(input, kPredictSize);
			featureRows.push_back(feature);
		}

		std::vector<int> maxIndexs;
		std::vector<float> maxVals;
		classify(featureRows, maxIndexs, maxVals, isChineseVec);

		for (size_t row_index = 0; row_index < input_size; row_index++) 
		{
			int index = maxIndexs[row_index];
			if (index < kCharactersNumber) 
			{
				outputs[row_index] = std::make_pair(kChars[index], kChars[index]);
			}
			else 
			{
				const char* key = kChars[index];
				std::string s = key;
				std::string province = m_map[s];
				//std::string province = kv_->get(s);
				outputs[row_index] = std::make_pair(s, province);
			}
		}
		return 0;
	}


#define HORIZONTAL    1
#define VERTICAL    0
#define NDEBUG

}	
