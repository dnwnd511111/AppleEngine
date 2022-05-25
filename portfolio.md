
# Portfolio

게임을 어떻게 만드는지 궁금해 하던 찰나에 DX11를 사용해서 게임을 만든다는 글을 보고 dx11(물방울)책을 사서 공부를 하다 그래픽스를 본격적으로 배우기로 마음먹었습니다.
책이나 튜토리얼로 간단한 그래픽스 이론들을 접하고 게임 엔진을 만들려고 했지만 책에서는 전반적인 게임 엔진 구조에 대한 내용이 찾기 힘들어 오픈 소스를 열어보기로 했습니다.
이때 깃헙에 별도 많고 개인이 만든 엔진인 Wicked Engine을 선택하고 구조나 피처들을 하나씩 공부했습니다. Graphics API(dx11, dx12 ...) 엔진 구조 등을 어느정도 파악한 후에 비교를 위해 다른 오픈 소스 엔진들을 열어보고 엔진에 기능을 추가했습니다. 엔진을 만들면서 엔진의 특정 피처에 집중할 시간이 부족하다는 것을 느끼고 AMD, NVIDIA에서 제공하는 후처리 라이브러리나 특정 피처 라이브러리를 (e.g FidelityFX, GameWorks ) 찾아보면서 공부하거나 엔진에 추가했습니다. 하지만 개인이 만든 오픈소스들은 구조적인 문제가 눈에 많이 보였기 때문에 엔진을 만드는 것을 접어두고 그래픽스 이론 공부를 하고 있습니다.

Wicked Engine의 전반적인 부분과, Hazel Engine의 GUI 부분을 기반으로 작성되었습니다.

피처에 대한 자세한 설명은 블로그에 기술되어 있습니다.(https://graphics12.tistory.com/)

## 간단한 설명

### RHI

- Dx11을 제거하고 Dx12 Low Level Api 기반으로 작성
- Command Queue를 3개로 나눠서 작업 (Graphics, Compute, Copy)
- 멀티스레딩을 위해 대부분의 자원들은 (지연 프레임 * Commandlist * Queue ) 또는 (지연 프레임 * Commandlist) 개수로 준비, 삭제되는 자원들은 지연 프레임 이후에 삭제
- C++쪽 Root Signature 생성 코드를 줄이고, 보기 쉽게 하기 위해 HLSL Root Signature를 사용
- Ring Buffer 방식의 Descriptor Heap을 사용
- 중복적인 Bind를 막기위해 현재 Bind 상태를 기록
- PSO를 중복해서 생성할 수 있기 때문에 해시 값을 이용해 Global로 관리
- Upload 버퍼는 크게 할당해서 나눠쓰는 방식


### Shader

- PBR (Isotropy, Anisotropy, Sheen, Clear Coat)
- 폭발적인 셰이더의 증가 때문에 Uber Shader 사용
- Bind를 쉽게 하기 위해서 Bindless Descriptor와 RootConstant를 사용
- Bindless 덕분에 정점 버퍼 바인드와 Input Layout을 생성할 필요가 없어짐
- 라이트 연산을 줄이기 위해 Tiled Shading 사용 (scalization)
- 모든 데이터는 압축 (Color, Normal같은 데이터는 8bit, UV는 16bit)
- Reversed z를 사용해 z fighting을 방지 
- 셰이더의 의존 헤더들(메타파일 생성)과 셰이더의 날짜를 비교해 셰이더 reload 


### 오브젝트 저장방식

Cache Hit를 늘리려고 DOD방식인 ECS를 사용
처음에는 ECS 라이브러리인 ENTT를 사용했지만 멀티스레드에서 어떻게 작동하는지 확실하지 않아 제거 후 간단한 ECS 제작


### CPU 업데이트 
gpu 자원의 업데이트에 사용할 데이터들을 업데이트

 1. 네트워크 처리
 2. 스크립팅 처리
 3. Input 처리
 4. Object의 Component들을 의존 관계에 따라 업데이트(여기에 Physics Simulation포함)
 5. Visibility 처리(Frustum Culling , Occlusion Query 결과 사용)


### GPU 업데이트, 랜더링

 1. 프레임에 쓰일 GPU 자원들 업데이트 및 Copy to GPU (Frame, Entity, Skining ...)
 2. Depth prepass(depth, primitive) 및 Occlusion Culling
 3. 랜더링된 Depth Buffer와 Primitive Buffer를 가지고 여러 버퍼들을 생성(velocity, linear depth, MSAA를 썼다면 resolve)
 4. Entity Culling(light, decal, env probe을 타일에 나눠담음)
 5. 위의 생성된 데이터를 이용해 랜더링에 쓰일 데이터를 생성(AO, SSR, Shadow Map, Planar Reflection ...)
 6. 이제 만들어진 데이터를 이용해서 Opaque 랜더링
 7. Transparent 랜더링
 8. Postprocess Chain (TAA, DOF, Motion Blur ...)
 9. 2D 랜더링


### 개선이 필요한 부분

- 환경맵의 indirect diffuse를 저장하지 않고 있음, spherical harmonics을 사용해서 저장하도록 수정해야함
- material editor에서 비주얼 스크립팅 부분에 imgui node 라이브러리를 사용, 문제가 많이 생겨서 제거 필요
- 언리얼은 postprocess chain이 없고 postprocess volume을 써서 chain을 구성하기 때문에 훨씬 유연함 
- 정확도를 위해서 tiled를 clustered 교체 해야함
- occupancy와 helper lane 등등의 이유로 인해 deferred로 교체해야 함
- software occlusion culling 추가
- nvidia의 waveworks가 DLL로 제공되서 수정이 불가능함
- 지형 관리 최적화가 필요함
- 언리얼처럼 render pass, 자원, 베리어를 자동으로 관리하는 시스템 필요
- ...


