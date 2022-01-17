
# Portfolio

DX11 -> DX12 -> DX12(DX11 제외) 순으로 제작
처음에는 DX11로 엔진을 만들고 거기에 DX12를 지원하는 방향으로 엔진을 설계 했는데 이 방식을 사용하면 DX12의 Bindless 같이 11에 없는 기능을 사용할 때 코드가 배가 되기 때문에 
로직에서 DX11을 지원하지 않는 방향으로 바꿨습니다.

Vulkan, DX12, Metal 같은 low level api에 적합한 구조로 설계


## 간단한 로직과 겪은 문제

### RHI

- 최대한 기본 API랑 비슷하게 구성
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



### 오브젝트 저장방식

Cache Hit를 늘리려고 DOD방식인 ECS를 사용했지만 상속구조로 된 데이터들은 크기가 일정하지 않아 사용할 수가 없는 문제가 생김
일부 컴포넌트들은 고정 크기로 저장하고 상속된 데이터들만 포인터로 저장하는 방식을 쓰려고 해봤지만 시리얼라이즈에서 문제가 생겨 아직까지 해결방안을 못 찾은 상태
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
- 언리얼은 postprocess chain이 없고 postprocess volume을 써서 chain을 구성하기 때문에 훨씬 유연함
문제는 언리얼 같이 유연한 postprocess volume을 구현하려면 material editor를 구현해야 한다. 이는 blueprint같은 비주얼 스크립팅과 이 스크립팅 노드들을 셰이더로 변환하는 부분을 구현하는 것을 의미해서 쉽지 않다.

- 정확도를 위해서 tiled를 clustered 교체 해야함
- occupancy와 helper lane 등등의 이유로 인해 deferred로 교체해야 함
- software occlusion culling 추가
- nvidia의 waveworks가 DLL로 제공되서 수정이 불가능함
- 언리얼처럼 지형을 쪼개서 관리하도록 해야함

