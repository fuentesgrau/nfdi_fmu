url_fmu_getdata="http://localhost:8080/api/v2/gateway/fmu/FMU_server/FMI_service/FMU_GetData"
url_fmu_dostep="http://localhost:8080/api/v2/gateway/fmu/FMU_server/FMI_service/FMU_DoStep"

for run in $(seq 1 100)
do
    curl $url_fmu_dostep -s -o /dev/nul
    curl $url_fmu_getdata -s -o /dev/null -X PUT
done

