#include "gdal_priv.h"
#include "util_external_dfal.h"
#define SIZE 1024
#include <string>
#include <iostream>
using namespace std;

// 从完整路径中得到文件名，不带扩展名
/*
*  @param	strFullPath    	[in] 完整路径
*  @param	result  	    [in] 文件名
*/
const char* getFilaNameFromPath(const char* strFullPath)
{
    string file_tmp_name = strFullPath;
    string file_name = file_tmp_name.substr(file_tmp_name.find_last_of("/")+1,file_tmp_name.find_last_of(".")-file_tmp_name.find_last_of("/")-1);
    return file_name.c_str();
}

int main(int argc,char* argv[])
{
//    const char* inputFile = "/media/835001CEF0E523DF/TestData/黑河流域2009-2011年地表蒸散发数据集/ET_Heihe_daily_2009/2009001_EvapoTranspiration.tif";
//    const char* outputFile = "/media/835001CEF0E523DF/TestData/黑河流域2009-2011年地表蒸散发数据集/ET_Heihe_daily_2009/2009001_EvapoTranspiration.hdf";
//
   const char* inputFile = argv[1];
    const char* outputFile = argv[2];
    GDALAllRegister();
    GDALDataset *poDatasetBand = (GDALDataset *) GDALOpen(inputFile, GA_ReadOnly );
    if(( poDatasetBand == NULL ))
    {
        printf("文件打开失败");
    }
    //投影信息
    char projection[SIZE];
    memset(projection, '\0', SIZE*sizeof(char));
    strcpy(projection, poDatasetBand->GetProjectionRef());
    double adfGeoTransform[6];
    poDatasetBand->GetGeoTransform(adfGeoTransform);
    //输入图像原始尺寸
    int nXSize=poDatasetBand->GetRasterXSize();
    int nYSize=poDatasetBand->GetRasterYSize();
    GDALRasterBand* pRasterData = poDatasetBand->GetRasterBand(1);
    float* data = new float[nXSize*nYSize];
    pRasterData->RasterIO(GF_Read,0,0,nXSize,nYSize,data,nXSize,nYSize,GDT_Float32,0,0);
//    for(int i = 0; i < 6 ;i ++)
//         cout<<adfGeoTransform[i]<<endl;

    // 创建FPAR
    DFALImage* pImageFPAR = CreateImageFile(outputFile,DFAL_AC_TRUNC,DFAL_LT_HDF5,"HDF5");
    // 根据LAI的图像属性写FPAR的属性
    {
        char temp_Rad_Path[100];
        pImageFPAR->SetStrMetadataX5("1000","SpatialResolution");
        pImageFPAR->SetStrMetadataX5("Geographic Lat/LON","Projection");
        pImageFPAR->SetStrMetadataX5(projection,"ProjectionStr");
        sprintf(temp_Rad_Path,"%f,%f,%f,%f,%f,%f",adfGeoTransform[0],adfGeoTransform[1],adfGeoTransform[2],adfGeoTransform[3],adfGeoTransform[4],adfGeoTransform[5]);
        pImageFPAR->SetStrMetadataX5(temp_Rad_Path,"ProjectionPara");
        pImageFPAR->SetStrMetadataX5(getFilaNameFromPath(inputFile),"QuanProductName");
        sprintf(temp_Rad_Path,"%d,%d%",nXSize,nYSize);
        pImageFPAR->SetStrMetadataX5(temp_Rad_Path,"size");
        DFALGroup* pGroupFPAR = pImageFPAR->CreateGroup("Group");

        DFALDataSet* pDataSet= pGroupFPAR->CreateDataSet(2,new int[2] {nXSize,nYSize},"DataSet_ET",DFAL_DT_Int16);
        short* dataout = new short[nXSize*nYSize];
        for(int i =0;i < nXSize*nYSize;i++)
        {
            if(data[i] > 32.767 || data[i] < -32.767)
            {
                cout<<"越界了"<<endl;
            }
            if( data[i] >(-9-0.00001) && data[i] < (-9+0.00001))
            {
                dataout[i] = -32767;
            }else
            {
                dataout[i] = (short)(data[i]*1000);
            }
        }
        pDataSet->WriteRaster(0,0,nXSize,nYSize,dataout,DFAL_DT_Int16,0,0);
        pDataSet->SetStrMetadataX5("-32767","FillValue");
        pDataSet->SetStrMetadataX5("1000","ScaleFactor");
        pDataSet->DeleteThis();
        pGroupFPAR->DeleteThis();
    }

    pImageFPAR->DeleteThis();
    delete[] data;
    GDALClose(poDatasetBand);
    return 0;
}
