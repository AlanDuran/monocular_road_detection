/*
 * otsu.c
 *
 *  Created on: 25 feb 2019
 *      Author: alan
 */
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "otsu.h"

using namespace cv;
using namespace std;

#define N_DIV 10

uint8_t get_threshold(Mat *src, int limit)
{
	Mat hist = *src;
	double q[2] = {0,0}, w_mean[2] = {0,0}, cv[2] = {0,0};
	double intra_cv[limit];
	int threshold,i;

	for (threshold = 0; threshold < limit; threshold++)
	{
		int index;
		for(index = 0; index <= 1; index++)
		{
			q[index] = 0;
			w_mean[index] = 0;
			cv[index] = 0;
		}

		for(i = 0; i < limit; i++)
		{
			if(i <= threshold)
			{
				q[0] += (double)hist.at<float>(i);
			}

			else
			{
				q[1] += (double)hist.at<float>(i);
			}
		}

		//To avoid getting a division to zero
		if(q[0] == 0) q[0] = 1;
		if(q[1] == 0) q[1] = 1;

		for(i = 0; i < limit; i++)
		{
			if(i <= threshold)
			{
				w_mean[0] += (i * (double)hist.at<float>(i)) / (q[0]);
			}

			else
			{
				w_mean[1] += (i * (double)hist.at<float>(i)) / (q[1]);
			}
		}

		for(i = 0; i < limit; i++)
		{
			if(i <= threshold)
			{
				cv[0] += ((i - w_mean[0])*(i - w_mean[0]))*(hist.at<float>(i)/q[0]);
			}

			else
			{
				cv[1] += ((i - w_mean[1])*(i - w_mean[1]))*(hist.at<float>(i)/q[1]);
			}
		}

		intra_cv[threshold] = q[0]*cv[0] + q[1]*cv[1];
	}

	//Get index of min element
	return distance(intra_cv,min_element(intra_cv, intra_cv + limit));
}

uint16_t get_horizon(Mat *src, int limit)
{
	Mat image = *src;
	Mat rec;
	Mat temp[N_DIV];

	/*
	char copy[] = "Imagen copia";
	namedWindow( copy, WINDOW_AUTOSIZE );
	imshow( copy, image );
	waitKey (100);
*/
	uint8_t tholds[N_DIV];
	uint16_t accum[N_DIV] = {0};
	double pdiff[N_DIV];

	int i,j;
	int segment = image.rows / N_DIV;

	//Calculate thresholds for each segment
	for(i = 0; i < N_DIV; i++)
	{
		Mat hist;
		int histSize = 256;
		float range[] = { 0, 255 }; //the upper boundary is exclusive
		const float* histRange = { range };
		bool uniform = true, accumulate = false;

		Range rows(i*segment, (i + 1)*segment);
		Range cols(0,image.cols);
		temp[N_DIV - i - 1] = image(rows,cols);

		char copy[] = "Imagen copia";

		calcHist(&temp[N_DIV - i - 1], 1, CV_HIST_ARRAY, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate );
		tholds[N_DIV - i - 1] = get_threshold(&hist, limit);
	}

	//Get the percentage of foreground (white) pixels for each threshold
	for(i = 0; i < N_DIV; i++)
	{
		for(j = 0; j < N_DIV; j++)
		{
			threshold( temp[i], temp[i], tholds[j], 255, THRESH_BINARY);
			accum[i] += 100*countNonZero(temp[i])/(temp[i].cols*temp[i].rows);
		}
	}

	//Calculate the percentage difference between segments
	for(i = 1; i <= N_DIV; i++)
	{
		double prom = (accum[i-1] + accum[i]) / 2;
		pdiff[i] = abs((accum[i] - accum[i-1]) / prom);
	}

	return segment * distance(pdiff,max_element(pdiff, pdiff + N_DIV));
}
