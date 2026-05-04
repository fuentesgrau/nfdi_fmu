docker build -t fmu_image .
docker run -p 50051:50051 fmu_image
