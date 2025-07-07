# Сборка: 

### Пререквизиты:

```bash
sudo apt-get install cmake g++ make ninja-build libgtest-dev lld
```

### Просто запустить все работающие тесты:

```bash
cd p-way-merge 
mkdir build && cd build 
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j`nproc` --target tests
ctest
```

### Или же для запуска конкретного теста:

Чтобы собрать тест с именем merge_test, нужно указать merge_test_ut в качестве target в вызове cmake --build, например эти команды собирают тесты из файла /pmerge/tests/merge_test.cpp: 

```bash
cd p-way-merge 
mkdir build && cd build 
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --target merge_test_ut
./merge_test_ut
```

На данный момент работают тесты merge_test, pack_test, simd_two_way_vector_resource_test и spilling_block_test.cpp.


### environment

выставляются для бинаря, не для билда.
PMERGE_FORCE_MUTE_STDOUT - отключить дебажные логи, с ними многие тесты сильно тормозят.
