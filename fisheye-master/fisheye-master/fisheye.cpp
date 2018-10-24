/************************************************************************
*	fisheye calibration
*	author ZYF
*	date 2014/11/22
************************************************************************/
//ģ�͵Ĳο����ĸ����ļ�����
//releaseģʽ���лᵼ�����ͼ�񱳾���Ϊ��ɫ����Щ�ط�������˵û��������أ���debugģʽ�޴�����

// FSQ���²��֣�������UndisImage���֣������Labelת������LabelTrans���������õ�����ת������LabelPoint
//				ʵ����ͼƬת����Labelת������������main

#include "fisheye.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

//�������� fsq2018.2.4
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using cv::Point2f;
using std::asin;
using namespace cv;
using namespace std;

#ifndef PI
#define PI (3.1415926)
#endif
/************************************************************************/
/* PointMap
/* ��һ������ͼ���ϵĵ��õȾ�ģ��ӳ�䵽��λ����
/************************************************************************/
void PointMap(Point2f sp, Point2f &dp, float r)
{
	PointMap(sp.x,sp.y,dp.x,dp.y,r);
}

/************************************************************************/
/* PointMap
/* ��һ������ͼ���ϵĵ��õȾ�ģ��ӳ�䵽��λ����
/* ������
/*		x,y: �������������ͼ���ϵĵ������
/*		new_x, new_y : �����������λ�����ϵ������
		theta_max ���۾�ͷ�ӳ���
		r  ����������ͼ��뾶
/************************************************************************/
void PointMap(float x, float y, float& new_x, float& new_y,float r)//ע�Ͳ��ֿ�������ʵ��ͼ����λ
{
	float l = sqrt(x*x + y*y);   //����ͼ��ĳ������ľ���	
	float theta_max = PI/2 ;     //���۾�ͷ���ӳ���
	
	//float x0 = 0;ԭͼ�ƶ�������
	//float y0 = 0;
	//float theta_change = atan2(y0 , x0);
	//float l_change = sqrt(x0*x0 + y0*y0);

	//�������alpha
	float alpha(0);
	if ( 0 == x) 
		alpha = PI / 2;
	else 
		alpha = atan2( y,x);

	float f = r / theta_max;    //�õȾ�ͶӰ�ķ�ʽ���㽹��f
	float theta = l / f;        //����ͼӳ�䵽��λԲ��theta = r/f����λԲr=1��
	float d = f*tan(theta);     //��λԲӳ�䵽ԭͼ

	//float tx = d* cos(alpha)-l_change*cos(theta_change);
	//float ty = d* sin(alpha) - l_change*sin(theta_change);
	float tx = d* cos(alpha);
	float ty = d* sin(alpha);

	//new_x = tx;
	//new_y = ty;

	//��һ��ȥ���ᵼ��ͼ�����ߴ��д�λ
	if ( x > 0)
		new_x = abs(tx);
	else if (x < 0)
		new_x = -abs(tx);
	else
		new_x = 0;

	if (y > 0)
		new_y = abs(ty);
	else if (y < 0)
		new_y = -abs(ty);
	else
		new_y = 0;
}

/************************************************************************/
/* PointMap2   δʹ��
/* ��һ������ͼ���ϵĵ�ӳ�䵽����ͼ���ϵ�һ���㣬ʹ��γ�Ȳ��䷨ӳ��
/* ������
/*		x,y: ���������������ͼ���ϵĵ������
/*		new_x, new_y : �������������ͼ���ϵ������
/*		r : ���������Բ�뾶
/************************************************************************/
void PointMap2(float x, float y, float& new_x, float& new_y, float r)
{
	float theta_x = x / r;
	float xx = r * sin(theta_x);
	float theta_y = y / r;
	float yy = r * sin(theta_y);

	//��������xx,yy
	float scale = 1.0f; // x,y��������ű�����Ĭ��Ϊ1�������˲�����ı�ӳ����
	int iters = 0;//
	for (int i = 0; i < iters; ++i) {
		float rr = sqrt(r*r - yy*yy);
		float xx1 = rr * xx / r;
		rr = sqrt( r*r - xx*xx);
		float yy1 = rr * yy / r;
		xx = xx1; yy = yy1;
	}

	if (x == 0)
		new_x = 0;
	else
		new_x = (x > 0 ? 1 : -1) * abs(xx);

	if (y == 0)
		new_y = 0;
	else
		new_y = (y > 0 ? 1 : -1) * abs(yy);
}

/************************************************************************/
/* ���ɴ�ԭͼ������ͼ��������ӳ�����mapx mapy  
������
	r �� Բ�뾶������ͼ��뾶
/************************************************************************/
void RectifyMap(Mat& mapx, Mat& mapy, float r)
{
	//int width = ceil(PI * r / 2) * 2;//ceil������ȡ���ڵ��ڱ��ʽ����С����

	//���������˱任����Ĵ�С��֮������ͼ��ͱ任����ͬ����
	//int width = 1000; //ӳ��ͼ��Ŀ��
	//float s = 480.0f / 720.0f; //ͼ��ߺͿ�ı���


	//���Ըı���������ͼ�񳤿��2018.7.10  ����������main��Ҳ�в����Ķ�
	int width = 1224; //ӳ��ͼ��Ŀ��
	float s = 480.0f / 720.0f; //ͼ��ߺͿ�ı���


	//����ͼ��ĸ� & ���ĵ�x��y����
	int height = width * s;
	int center_x = width / 2, center_y = height / 2;
	//����mapx mapy ��Ϊ32λ����
	mapx.create(height,width,CV_32F);
	mapy.create(height,width,CV_32F);

	for (int i = 0; i < height; ++i) 
	{
		//����ȷ��
		float y = center_y - i;
		float* px = (float*)(mapx.data + i * mapx.step);//��i�е�һ�����صĵ�ַ
		float* py = (float*)(mapy.data + i * mapy.step);
		for (int j = 0; j < width; ++j) 
		{
			float x = j - center_x;
			float nx,ny;
			//����ͼ����Բ��������߲����ͼ������
			//Բ�������ڵ���PointMap���õȾ�ģ�ͽ�����ͼ���ϵĵ�ӳ�䵽��λ����
			if (sqrt(x*x + y*y) >= r)//ʵ�������if���ã���ȥ��
			{
				nx = -1;
				ny = -1;
			}
			else
			{
				PointMap(x, y, nx, ny,r);//
				//PointMap2(x, y, nx, ny, 300);//
				px[j] = nx;
				py[j] = ny;
			}
		}
	}
}

/************************************************************************/
/* ����ͼ��   fsq2018.1.27                                     
/************************************************************************/
void UndisImage(Mat undistort_image, Mat& distort_image, Mat mapx, Mat mapy)
{
	assert(mapx.rows == mapy.rows && mapy.cols == mapy.cols);
	//�任ǰͼ��� ��&����λ��
	int height = undistort_image.rows;
	int width = undistort_image.cols;
	float cx = width / 2;
	float cy = height / 2;
	//cout << width << endl << height << endl;
	//cx = 320; cy = 260;

	//�任��ͼ��� ��&����λ��
	int distort_height = mapx.rows;
	int distort_width = mapy.cols;
	float center_x = distort_width / 2;
	float center_y = distort_height / 2;

	distort_image.create(distort_height, distort_width, undistort_image.type());
	distort_image.setTo(0);
	int channel = distort_image.channels();
	cv::Mat_<cv::Vec3b> _undistrot_image = undistort_image;
	cv::Mat_<float> _mapx = mapx;
	cv::Mat_<float> _mapy = mapy;

	for (int i = 0; i < distort_height; ++i) 
	{
		uchar* pdata = distort_image.data + i * distort_image.step;
		//float* pmapx = (float*)(mapx.data + i * mapx.step);
		//float* pmapy = (float*)(mapy.data + i * mapy.step);
		for (int j = 0; j < distort_width; ++j) 
		{
// 			if ((i - center_y)*(i - center_y) + (j - center_x)*(j - center_x) > un_width * un_width / 4) {
// 				continue;
// 			}
			//int x = pmapx[j] + cx;
			//int y = cy - pmapy[j];
			int x = _mapx(i,j) + cx;
			int y = cy - _mapy(i,j);
			//����ͼԲ���ⲻ�������
			if (x - cx == -1 && cy - y == -1)
			{
				continue;
			}
			//��Բ��������ĳλ�ã���Ӧԭͼ�ϳ�����Χ����������أ����Ǻڱߵ�����
			if ((x < 0 || x >= width || y < 0 || y >= height)) 
			{
				continue;
			}
			for (int k = 0; k < channel; ++k) 
			{
				pdata[j * channel + k] = _undistrot_image(y,x)[k];
			}
		}
	}
	//cv::resize(distort_image,distort_image,undistort_image.size());
}


/************************************************************************/
/*����Labelת��������任��PointMap�����ķ�������fsq2018.2.4
/*������
/*    x,y : �����������ת����ԭͼ����
/*    new_x,new_y : ���ֵ��ת���������ͼƬ����Ӧ������
/*    r : ������������۰뾶
/************************************************************************/
void LabelPoint(float x, float y, float& new_x, float& new_y, float r)
{
	float theta_max = PI / 2;	//���۾�ͷ���ӳ���
	float d = sqrt(x*x + y*y);

	float alpha(0);
	if (0 == x)
		alpha = PI / 2;
	else
		alpha = atan2(y, x);

	float f = r / theta_max;
	float l = f * atan2(d, f);
	new_x = l * cos(alpha);
	new_y = l * sin(alpha);
}


/************************************************************************/
/* Labelת�� fsq2018.2.4
/* ������
/*     x1,x2,y1,y2 : ���������ԭͼLabel��2D�����ϡ����¶�������
/*     bbx1,bbx2,bby1,bby2 : ���ֵ������ͼƬ����Ӧ��2D�����ϡ����¶�������
/************************************************************************/
void LabelTrans(float x1, float x2, float y1, float y2, float& bbx1, float& bbx2, float& bby1, float& bby2)
{
	float nx1, nx2, nx3, nx4, ny1, ny2, ny3, ny4;
	//float cx = 616, cy = 186, center_x = 500, center_y = 333;
	//����2018.7.10  �ö������滻��һ�䣬�Ա��ڸ�������ͼƬ�ߴ����Ӧ�ı�label�任����
	float cx = 616, cy = 186;	
	int width = 1000; //ӳ��ͼ��Ŀ��
	float s = 480.0f / 720.0f; //ͼ��ߺͿ�ı���
	int height = width * s;
	float center_x = width / 2, center_y = height / 2;

    //��ԭͼ����ϵԭ������ͼƬ���ģ�Ϊ��ӦͼƬ�����ֵ�Resize����*2����
	x1 = 2*(x1 - cx);
	x2 = 2*(x2 - cx);
	y1 = 2*(cy - y1);
	y2 = 2*(cy - y2);
	//��ԭͼ2D���ĸ���������ת��Ϊ����ͼƬ����Ӧ����
	LabelPoint(x1, y1, nx1, ny1, 200);//����
	nx1 = center_x + nx1;
	ny1 = center_y - ny1;
	//cout << nx1 << endl << ny1 << endl<<endl;
	
	LabelPoint(x2, y1, nx2, ny2, 200);//����
	nx2 = center_x + nx2;
	ny2 = center_y - ny2;
	//cout << nx2 << endl << ny2 << endl << endl;

	LabelPoint(x1, y2, nx3, ny3, 200);//����
	nx3 = center_x + nx3;
	ny3 = center_y - ny3;
	//cout << nx3 << endl << ny3 << endl << endl;	

	LabelPoint(x2, y2, nx4, ny4, 200);//����
	nx4 = center_x + nx4;
	ny4 = center_y - ny4;
	//cout << nx4 << endl << ny4 << endl << endl;

	// �Ƚ�����ֵ������ȷ������ͼƬ��2D���ο�
	bbx1 = min(nx1, nx3);
	bby1 = min(ny1, ny2);
	bbx2 = max(nx4, nx2);
	bby2 = max(ny4, ny3);
}

//���������� main

void main()
{
	Mat undistort_image = imread("000145.png");	
	Mat mapx, mapy, distort_image;
	//imshow("origin_image", undistort_image);

	//�Ŵ�ͼƬ����ԭͼӳ�䵽��λ�����ϵķ�Χ���󣬽����õ�λ����ӳ�䵽����ͼ�ϵ��������ͬ���ɼ��ٺڱ�
	resize(undistort_image, undistort_image, Size(undistort_image.cols * 2, undistort_image.rows * 2));
	//imshow("resized_image", undistort_image);

	//�޸�r����������ͼ��Բ�뾶����ҪӰ���Ǹı��˽���f���ı���ԭͼӳ�䵽��λ�����ϵķ�Χ�������ɵ����ڱ�
	RectifyMap(mapx, mapy, 200);

	//���Ըı���������ͼ�񳤿��2018.7.10 RectifyMap��Ҳ�в����Ķ�
	RectifyMap(mapx,mapy,800);
	//imshow("mapx", mapx);
	//imshow("mapy", mapy);
	UndisImage(undistort_image, distort_image, mapx, mapy);
	imshow("distort_iamge", distort_image);
	imwrite("000145.jpg",distort_image);
	waitKey();

}


//��������main fsq2018.2.4
/*
int main()
{
	Mat undistort_image;
	Mat mapx, mapy, distort_image;	
	string fileName,fileName_label;
	//ͼƬ�б��ĵ�&���������ַ
	char* filePath = "E:\\FishEye\\dir.txt";//���������ļ��������ĵ���ַ
	char* dir_in = "F:\\KITTI\\training\\image_2\\";//�������������ļ���ַ
	char* dir_out = "E:\\FishEye_out\\image_2\\";//���������ļ������ַ
	//Label�б��ĵ�&���������ַ
	char* filePath_label = "E:\\FishEye\\dir_label.txt";
	char* dir_label_in = "F:\\KITTI\\training\\label_2\\";
	char* dir_label_out = "E:\\FishEye_out\\label_2\\";

	//ͼƬת��
	
	RectifyMap(mapx, mapy, 200);//����ͼƬλ��ӳ�����

	int counts = 0;//�����ʾ����
	cout << "Pic Trans" << endl;

	ifstream inFile(filePath);
	if (!inFile.is_open())
	{
		cerr << "Failed open the file" << endl;
		return -1;
	}
	while (getline(inFile, fileName)) //���ж�ȡ�ļ���
	{
		string str_in = dir_in + fileName;
		string str_out = dir_out + fileName;
		//��ת��ͼƬ����
		undistort_image = imread(str_in, 1);
		//ͼƬת��
		resize(undistort_image, undistort_image, Size(undistort_image.cols * 2, undistort_image.rows * 2));
		UndisImage(undistort_image, distort_image, mapx, mapy);
		//����ͼƬ���� & ������ʾ
		imwrite(str_out, distort_image);
		counts += 1;
		cout << counts<<endl;
	}
	inFile.close();		
	

	//Labelת��
	cout << "Label Trans" << endl;
	int counts_label = 0;

	ifstream inFile_label(filePath_label);
	if (!inFile_label.is_open())
	{
		cerr << "Failed open the file" << endl;
		return -1;
	}
	while (getline(inFile_label, fileName_label)) //���ж�ȡ�ļ���
	{
		string str_label_in = dir_label_in + fileName_label;
		string str_label_out = dir_label_out + fileName_label;
		//����&���label�ļ�
		ifstream inLabel(str_label_in);
		ofstream outLabel(str_label_out);

		string label;
		string str_type, str_truncated, str_occluded, str_alph, str_bbx1, str_bby1, str_bbx2, str_bby2;
		//���ж�ȡ�����ļ�����
		//��ȡ�������ݣ����2D������&���¶�������
		//������ת����������ͼ��2D�����꣬������label�ļ�
		while (getline(inLabel, label))
		{
			istringstream is(label);//���ո�ָ����ݣ�str_type���str_bbx1 str_bbx2 str_bby1 str_bby2Ϊ2D�����Ϻ����¶���
			is>> str_type >> str_truncated >> str_occluded >> str_alph >> str_bbx1 >> str_bby1 >> str_bbx2 >> str_bby2;

			float x1 = atof(str_bbx1.c_str());//��������ת��string --> float
			float x2 = atof(str_bbx2.c_str());
			float y1 = atof(str_bby1.c_str());
			float y2 = atof(str_bby2.c_str());
			
			float bbx1, bbx2, bby1, bby2;
			LabelTrans(x1, x2, y1, y2, bbx1, bbx2, bby1, bby2);
			
			outLabel << str_type << " " <<str_truncated<<" "<<str_occluded<<" "<< str_alph<<" "<< bbx1 << " " << bby1 << " " << bbx2 << " " << bby2 << endl;
		}
		inLabel.close();
		outLabel.close();
		counts_label += 1;
		cout << counts_label << endl;
	}
	inFile_label.close();
	waitKey(0);
}
*/