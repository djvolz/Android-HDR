#include <string>
#include <opencv2/opencv.hpp>
#include "NativeLogging.h"



using namespace cv;

static const char* TAG = "CreateHDR";

void read_input_images(vector<string>& inputImagePaths, vector<Mat>& images)
{
    /* Read the images at the locations in inputImagePaths
     * and store them in the images vector.
     * Helpful OpenCV functions: imread.
     */
	for(auto imgPath : inputImagePaths)
		images.push_back(imread(imgPath));


}

void align_images(vector<Mat>& images)
{
    /* Update the images vector with aligned images.
     * Helpful OpenCV classes and functions:
     * ORB, BFMatcher, findHomography, warpPerspective
     * The above classes and functions are just recommendations.
     * You are free to experiment with other methods as well.
     */

     int numImgs = images.size();
     int k = 1;

     /* Descriptors. */
	 vector<Mat> des(numImgs);

     /* std::vector<cv::KeyPoint> keypoints_1, keypoints_2; */
	 vector<vector<KeyPoint>> kps(numImgs);


     /* # Initiate ORB detector */
	 ORB orbDetector(4096);


     /**
      * Example:
      * create BFMatcher object
      * bf = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)
      */
	 BFMatcher bfMatcher (NORM_HAMMING, true);


     orbDetector(images[0], Mat(), kps[0], des[0]);

     for(int i = 1; i < numImgs; i++) {
         /* Declarations */
         Mat homographyMat;
         Mat warpMat;

         vector<vector<DMatch> > good;
         vector<Point2f> src_pts;
         vector<Point2f> dst_pts;


         orbDetector(images[i], Mat(), kps[i], des[i]);


         /* Finds the k best matches for each descriptor from a query set. */
         bfMatcher.knnMatch(des[i], des[0], good, k);

        /*
         * Example:
         * src_pts = np.float32([ kp1[m.queryIdx].pt for m in good ]).reshape(-1,1,2)
         * dst_pts = np.float32([ kp2[m.trainIdx].pt for m in good ]).reshape(-1,1,2)
         */
        for(int j = 0; j < good.size(); j++) {
            if(0 < good[j].size()) {
                src_pts.push_back(kps[i][good[j][0].queryIdx].pt);
                dst_pts.push_back(kps[0][good[j][0].trainIdx].pt);
            }
        }

        /* Example: M, mask = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC,5.0) */
        homographyMat = findHomography(src_pts, dst_pts, CV_RANSAC, 5.0);


        warpPerspective(images[i], warpMat, homographyMat, images[i].size());

        /* Return the warped image. */
        images[i] = warpMat;

    }
         

}

void compute_weights(vector<Mat>& images, vector<Mat>& weights)
{
    /*
     *  Compute the weights for each image and place them in the given weights vector.
     *  Helpful OpenCV functions: Laplacian, split, merge.
     *
     *  Methods described in this paper:
     *      http://research.edm.uhasselt.be/tmertens/papers/exposure_fusion_reduced.pdf
     */
 
    int ddepth = CV_32F;
    float divisor;

    for(auto src : images) {
        Mat dst_contrast   (src.rows, src.cols, ddepth);
        Mat dst_saturation (src.rows, src.cols, ddepth);
        Mat dst_exposure   (src.rows, src.cols, ddepth);
        Mat dst_weights    (src.rows, src.cols, ddepth);

        vector<Mat> rgbValues;
        vector<Mat> sqValues;

        int num_color_channels = 3;

        float mean, meanSqDiff, stdDev;


        /*************
         * Contrast
         *************/

        /* We apply a Laplacian filter to the grayscale version of each image */

        /* Example: Laplacian( src_gray, dst, ddepth, kernel_size, scale, delta, BORDER_DEFAULT ); */
        Mat greyMat;
        cv::cvtColor(src, greyMat, CV_BGR2GRAY);
        Laplacian(greyMat, dst_contrast, ddepth, 1, 1, 0, BORDER_DEFAULT);



         /*************
          * Saturation
          *************/

          /* We include a saturation measure S,
           * which is computed as the standard deviation
           * within the R, G and B channel, at each pixel.
           */
         split (src, rgbValues);
         split (src, sqValues);


         for (int i = 0; i < src.rows; i++) {
             for (int j = 0; j < src.cols; j++) {
                mean = 0.0;

                 /* 1. Work out the Mean (the simple average of the numbers). */
                 mean = (float)((int)rgbValues[0].at<float>(i,j)
                               +(int)rgbValues[1].at<float>(i,j)
                               +(int)rgbValues[2].at<float>(i,j));
                 mean /= num_color_channels;


                 sqValues[0].at<float>(i,j) = rgbValues[0].at<float>(i,j) - mean;
                 sqValues[1].at<float>(i,j) = rgbValues[1].at<float>(i,j) - mean;
                 sqValues[2].at<float>(i,j) = rgbValues[2].at<float>(i,j) - mean;
                /* 2. Then for each number: subtract the Mean and square the result. */
                sqValues[0].at<float>(i,j) = pow(sqValues[0].at<float>(i,j), 2.0);
                sqValues[1].at<float>(i,j) = pow(sqValues[1].at<float>(i,j), 2.0);
                sqValues[2].at<float>(i,j) = pow(sqValues[2].at<float>(i,j), 2.0);

                /* 3. Then work out the mean of those squared differences. */
                meanSqDiff =  (float)((int)sqValues[0].at<float>(i,j)
                                     +(int)sqValues[1].at<float>(i,j)
                                     +(int)sqValues[2].at<float>(i,j));
                meanSqDiff /= (float)num_color_channels;

                /* 4. Take the square root of that and you are done. */
                stdDev = sqrt(meanSqDiff);
                dst_saturation.at<float>(i,j) = stdDev;
             }
         }

     

         /*************
          * Exposure
          *************/

          /*
           * We want to keep intensities that are not near zero (underexposed) or one (overexposed).
           * We weight each intensity i based on how close it is to 0.5 using a
           * Gauss curve: exp(−(i−0.5)^2/2σ^2)􏰁, where σ equals 0.2 in our implementation.
           * To account for multiple color channels, we apply the Gauss curve to each channel separately,
           * and multiply the results, yielding the measure E.
           */
         for (int i = 0; i < src.rows; i++) {
             for (int j = 0; j < src.cols; j++) {
                 float exponent[num_color_channels];
                 for (int k = 0; k < num_color_channels; k++) {
                     float temp_exp = (-1) * (pow(((float)rgbValues[k].at<float>(i,j) - 0.5),2) / (2*(0.2*0.2)));
                     exponent[k] = exp(temp_exp);
                 }
                 dst_exposure.at<float>(i,j) = exponent[0] * exponent[1] * exponent[2];
             }
         }


         /*************
          * Fusion
          *************/

         /*
          * For each pixel, we combine the information from
          * the different measures into a scalar weight map using multiplication.
          */

         for (int i = 0; i < dst_weights.rows; i++) {
             for (int j = 0; j < dst_weights.cols; j++) {
                 dst_weights.at<float>(i,j) = //dst_contrast.at<float>(i,j)   *
                                              //dst_saturation.at<float>(i,j) *
                                              dst_exposure.at<float>(i,j);
             }
         }


         /* Push weight of image onto weights vector. */
         weights.push_back(dst_weights);
     }



     /*************
      * Normalize
      *************/

     /*
      * To obtain a consistent result, we normalize the values
      * of the N weight maps such that they sum to one at each pixel (i, j).
      */
     for (int i = 0; i < images[0].rows; i++) {
         for (int j = 0; j < images[0].cols; j++) {
             /* Reset denominator. */
             divisor = 0.0;
             for (auto &weight : weights){
                 divisor += weight.at<float>(i,j);
             }
             for (auto &weight : weights) {
                 if (divisor) {
                     weight.at<float>(i,j) /= divisor;
                 }
             }
         }
     }


}


void blend_pyramids(int numLevels, vector<Mat>& images, vector<Mat>& weights, Mat& output)
{
    /*
     *  Blend the images using the calculated weights.
     *  Store the result of exposure fusion in the given output image.
     *  Helpful OpenCV functions: buildPyramid, pyrUp.
     */

    /* 
     * Example: Image Pyramids
     *
     * http://docs.opencv.org/trunk/doc/py_tutorials/py_imgproc/py_pyramids/py_pyramids.html
     */

    vector<Mat> temp_for_sizing;
    vector<Mat> outPyr(numLevels);

    /* Example: void buildPyramid(InputArray src, OutputArrayOfArrays dst, int maxlevel, int borderType=BORDER_DEFAULT) */
    buildPyramid(weights[0], temp_for_sizing, numLevels);

    for(int i = 0; i < numLevels; i++)
        outPyr[i] = Mat::zeros(temp_for_sizing[i].rows, temp_for_sizing[i].cols, CV_32FC3);
    for(int i = 0; i < images.size(); i++) {
        vector<Mat> weightPyr(numLevels);
        vector<Mat> gaussPyr (numLevels);
        Mat image;

        /* Create weights pyramid. */
        buildPyramid(weights[i], weightPyr, numLevels);

        image = images[i];

         /* Generate Gaussian pyramid. */
         buildPyramid(image, gaussPyr, numLevels -1);

         /* Generate Laplacian pyramid. */
         for (int j = 1; j < numLevels; j++) {
             Mat laplacianPyr;
             pyrUp(gaussPyr[j], laplacianPyr);

             /* Resize necessary if the images weren't aligned at start. */
             resize(laplacianPyr, laplacianPyr, gaussPyr[j - 1].size());
             subtract(gaussPyr[j - 1], laplacianPyr, gaussPyr[j - 1]);
         }
    
        /* Apply weights. */
        for (int k = 0; k < numLevels; k++) {
           vector<Mat> rgbValues;
           split(gaussPyr[k], rgbValues);
           for (auto &color : rgbValues)
               multiply(color, weightPyr[k], color);
           
           merge(rgbValues, gaussPyr[k]);
           add(gaussPyr[k], outPyr[k], outPyr[k]);
        }
    }

    /* Combine laplacian. */
    output = outPyr[outPyr.size()-1];
    for (int i = numLevels - 2; i >= 0; i--) {
        Mat laplacianPyr;
        pyrUp(output, laplacianPyr);

        /* Resize necessary if the images weren't aligned at start. */
        resize(laplacianPyr, laplacianPyr, outPyr[i].size());
        add(outPyr[i], laplacianPyr, output);
    }
}

/**
 * Convert uchar to float. 
 */
void convertUCtoF(vector<Mat>& images){
    for (auto &image : images) {
        Mat temp(image.rows, image.cols, CV_32FC3);
        for (int x = 0; x < image.rows; x++)
            for (int y = 0; y < image.cols; y++)
                for (int rgb = 0; rgb < 3; rgb++)
                    temp.at<Vec3f>(x,y)[rgb] = image.at<Vec3b>(x,y)[rgb]/255.0;

        image = temp;
    }
}

/**
 * Convert float to uchar. 
 */
void convertFtoUC(Mat& image){
    //Convert image pixels from float to unsigned char
    Mat temp(image.rows,image.cols,CV_8UC3);
    for (int x = 0; x < image.rows; x++) 
        for (int y = 0; y < image.cols; y++) 
            for (int rgb = 0; rgb < 3; rgb++) 
                temp.at<Vec3b>(x,y)[rgb] = saturate_cast<uchar>(image.at<Vec3f>(x,y)[rgb]*255.0);

    image = temp;
}

bool CreateHDR(vector<string> inputImagePaths, string outputImagePath)
{
    //Read in the given images.
    vector<Mat> images;
    read_input_images(inputImagePaths, images);

    //Verify that the images were correctly read.
    if(images.empty() || images.size()!=inputImagePaths.size())
    {
        LOG_ERROR(TAG, "Images were not read in correctly!");
        return false;
    }

    //Verify that the images are RGB images of the same size.
    const int numChannels = 3;
    Size imgSize = images[0].size();
    for(const Mat& img: images)
    {
        if(img.channels()!=numChannels)
        {
            LOG_ERROR(TAG, "CreateHDR expects 3 channel RGB images!");
            return false;
        }
        if(img.size()!=imgSize)
        {
            LOG_ERROR(TAG, "HDR images must be of equal sizes!");
            return false;
        }
    }
    LOG_INFO(TAG, "%d images successfully read.", images.size());

    //Now we'll make sure that the images line up correctly.
    align_images(images);
    LOG_INFO(TAG, "Image alignment complete.");

    // Change pixel values from uchar to float.
    convertUCtoF(images);

    //Compute the weights for each image.
    vector<Mat> weights;
    compute_weights(images, weights);
    if(weights.size()!=images.size())
    {
        LOG_ERROR(TAG, "Image weights were not generated!");
        return false;
    }
    LOG_INFO(TAG, "Weight computation complete.");

    //Fusion using pyramid blending.
    int numLevels = 9;
    Mat outputImage;
    blend_pyramids(numLevels, images, weights, outputImage);
    if(outputImage.empty())
    {
        LOG_ERROR(TAG, "Blending did not produce an output!");
        return false;
    }
    LOG_INFO(TAG, "Blending complete!");

     // Change pixel values from float to uchar.
    convertFtoUC(outputImage);


    //Save output.
    bool result = imwrite(outputImagePath, outputImage);
    if(!result)
    {
        LOG_ERROR(TAG, "Failed to save output image to file!");
    }
    return result;
}
