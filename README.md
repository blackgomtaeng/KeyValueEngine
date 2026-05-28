# KeyValueEngine

# 윈도우 내에서 '#' 다음 줄에서 드래그를 하여 윈도우 터미널인 cmd(Command, 커맨드) & pw(Powershell, 파워셀)을 접근하려면?
# https://cmake.org/download/을 웹주소창에 넣어주세요.
# 최신버전으로 설치하셔도 좋지만 사양에 맞춰서 하려면 AI(제미나이, chat gpt, copilot 등)를 접근하여 해당 사양을 넣어서 무엇을 사용하면 가장 좋은가라는 식으로 언급해보세요.
# https://www.mingw-w64.org/downloads/와 https://www.mingw-w64.org/source/ 중에 원하는 것을 선택하여 사용하세요.
# https://www.msys2.org/을 설치하세요.
# 위 주소를 접근하여 설치한다면 문제점들이 없어질 것입니다. 그리고 윈도우에서는 왠만한 문제들 중 하나가 환경설정하는 방법이니 윈도우에서는 꼭 필요한 존재이므로 제대로 설치하고 설정하시기 바랍니다.

# 만약 위에 있는 방법들이 귀찮다면 윈도우 내에서는 visual studio 최신버전을 설치해주세요. 블로그나 다른 곳에서 최신버전과 달리 구버전을 설치해라고 하는 경우가 있는데요. 회사 양식에 맞춰야 한다면 구버전을 구하여 설치하시고 그외에 양식이 아니어도 상관이 없다면 최신버전으로 접근하여 설치하시기 바랍니다.

# Windows = CMakelists.txt 파일을 실행하는 빌드 명령어
mkdir build ; cd build                      # 1. 빌드 전용 디렉토리 생성 및 이동
cmake .. ; cmake --build . --config Release # 2. 크로스플랫폼 메이크파일 구성 및 최종 컴파일 (한 줄로 결합 실행)
move Release\* . ; .\kve.exe                # 3. 산출된 결과물을 상위 실행 위치로 가져와서 가동


# Linux/Unix = CMakelists.txt 파일을 실행하는 빌드 명령어
mkdir -p build && cd build                  # 1. 빌드 전용 디렉토리 생성 및 이동
cmake .. && cmake --build .                 # 2. 빌드 환경 구성 및 컴파일 수행 (한 줄로 결합 실행)
./kve                                       # 3. 엔진 프로그램 최종 실행
