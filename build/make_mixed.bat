@set OUT=.\mixed\release\
@if exist %OUT% (
@pushd %OUT%
@del *.* /Q
@popd
)

@xcopy .\x86\Release\libopentrack-tracker-deepface.dll %OUT% /Y

@set OUT=.\mixed\release\deepfacetrack\
@if exist %OUT% (
@pushd %OUT%
@del *.* /Q
@popd
)

@xcopy ..\3rdParty\data %OUT% /E /Y

@xcopy .\x64\Release\deep_face_track_recognition.exe %OUT% /Y
@xcopy .\x64\Release\gpu_info.exe %OUT% /Y
@xcopy .\x86\Release\deep_face_track_camera.exe %OUT% /Y

@set OPENCV_PATH=..\3rdParty\opencv\x64\vc14\bin
@xcopy %OPENCV_PATH%\opencv_core345.dll %OUT% /Y
@xcopy %OPENCV_PATH%\opencv_dnn345.dll %OUT% /Y
@xcopy %OPENCV_PATH%\opencv_imgcodecs345.dll %OUT% /Y
@xcopy %OPENCV_PATH%\opencv_imgproc345.dll %OUT% /Y
@xcopy %OPENCV_PATH%\opencv_highgui345.dll %OUT% /Y
@xcopy %OPENCV_PATH%\opencv_videoio345.dll %OUT% /Y

@rem set PLAIDML_PATH=..\3rdParty\plaidml\lib
@rem xcopy %PLAIDML_PATH%\plaidml.dll %OUT% /Y