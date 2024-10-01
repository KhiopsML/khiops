
# Export environment variables to MPI processes
for line in $(env | grep -E '^(KHIOPS|Khiops|AWS_|S3_|GOOGLE_)'); do
    name=${line%%=*}
    MPI_EXTRA_FLAGS="${MPI_EXTRA_FLAGS} -x ${name}"
done