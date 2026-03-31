//
// Created by 曹旭峰 on 2025/12/17.
//

#ifndef JINGAN_MEDIA_SAFEAWSINCLUDE_H
#define JINGAN_MEDIA_SAFEAWSINCLUDE_H

#if defined(ENABLE_S3_STORAGE)
#pragma once

// ========== 清理 X11 / 旧 C 库常见污染宏 ==========
#ifdef None
#undef None
#endif

#ifdef Status
#undef Status
#endif

#ifdef Bool
#undef Bool
#endif

#ifdef Success
#undef Success
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef Always
#undef Always
#endif

#ifdef Never
#undef Never
#endif


#include <aws/s3/S3Client.h>
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/core/utils/UUID.h>

#endif

#endif //JINGAN_MEDIA_SAFEAWSINCLUDE_H
