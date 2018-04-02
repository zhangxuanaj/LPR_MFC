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
		//ӳ���
		typedef pair<string, string> CodeProvince;

		if (m_map.empty()) {
			m_map.insert(CodeProvince("zh_chuan", "��"));
			m_map.insert(CodeProvince("zh_e", "��"));
			m_map.insert(CodeProvince("zh_gan", "��"));
			m_map.insert(CodeProvince("zh_gan1", "��"));
			m_map.insert(CodeProvince("zh_gui", "��"));
			m_map.insert(CodeProvince("zh_gui1", "��"));
			m_map.insert(CodeProvince("zh_hei", "��"));
			m_map.insert(CodeProvince("zh_hu", "��"));
			m_map.insert(CodeProvince("zh_ji", "��"));
			m_map.insert(CodeProvince("zh_jin", "��"));
			m_map.insert(CodeProvince("zh_jing", "��"));
			m_map.insert(CodeProvince("zh_jl", "��"));
			m_map.insert(CodeProvince("zh_liao", "��"));
			m_map.insert(CodeProvince("zh_lu", "³"));
			m_map.insert(CodeProvince("zh_meng", "��"));
			m_map.insert(CodeProvince("zh_min", "��"));
			m_map.insert(CodeProvince("zh_ning", "��"));
			m_map.insert(CodeProvince("zh_qing", "��"));
			m_map.insert(CodeProvince("zh_qiong", "��"));
			m_map.insert(CodeProvince("zh_shan", "��"));
			m_map.insert(CodeProvince("zh_su", "��"));
			m_map.insert(CodeProvince("zh_jin", "��"));
			m_map.insert(CodeProvince("zh_wan", "��"));
			m_map.insert(CodeProvince("zh_xiang", "��"));
			m_map.insert(CodeProvince("zh_xin", "��"));
			m_map.insert(CodeProvince("zh_yu", "ԥ"));
			m_map.insert(CodeProvince("zh_yu1", "��"));
			m_map.insert(CodeProvince("zh_yue", "��"));
			m_map.insert(CodeProvince("zh_yun", "��"));
			m_map.insert(CodeProvince("zh_zang", "��"));
			m_map.insert(CodeProvince("zh_zhe", "��"));
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
			Mat feature = charFeatures(charInput, kPredictSize); //���������charInput����������������charInput�ͷֱ��ʳߴ�ΪkPredictSize*kPredictSize��10��10��������ǵ��о���
			featureRows.push_back(feature); //Mat��push_back������Ϊmatʱ��feature����feature����������Mat����featureRows������ͬ
			//featureRowsÿ����ÿ���ֿ����������
		}

		cv::Mat output(charVecSize, kCharsTotalNumber, CV_32FC1);
		ann_->predict(featureRows, output);//�����緽��predict����������������mat�ࣨ����charVec���ֿ�Ľ����ÿ����һ���ֿ�����������������mat��
		//output��Ӧ�ô��Ÿ���ֵ�����������зǼ���ֵ����
		//���outputĳ�У���Ӧĳ���ֿ��Ԥ���������Ԫ��ֵ����С����������ֿ�ܿ��ܲ�����

		for (size_t output_index = 0; output_index < charVecSize; output_index++) 
		{
			CCharacter& character = charVec[output_index];
			Mat output_row = output.row(output_index); //outputÿ��Ӧ����ÿ���ֿ��ann���������ÿ��Ԫ���Ƕ�ÿ��������ĸ���ֵ���Ӧ�����ʣ�����������Ϊ�����ֿ�������

			int result = -1;
			float maxVal = -2.f;//maxVal�Ǹ������ֵ
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

	//�ж��ֿ��Ƿ��Ǻ���
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
