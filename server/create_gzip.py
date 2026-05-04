import os
import gzip
import shutil

source_dir = r"D:/Stockage/Desktop/jdc/Projets/gestion_arrosage_jdc/server/www"
output_dir = r"D:/Stockage/Desktop/jdc/Projets/gestion_arrosage_jdc/server/output"

for root, dirs, files in os.walk(output_dir):
    for file in files:
        print(file)
        os.remove(file)

if False:
    for root, dirs, files in os.walk(source_dir):
        for file in files:
            input_path = os.path.join(root, file)

            # Build relative path
            rel_path = os.path.relpath(input_path, source_dir)
            output_path = os.path.join(output_dir, rel_path + ".gz")

            # Create output directory if needed
            os.makedirs(os.path.dirname(output_path), exist_ok=True)
            print(output_path)

            # Compress file
            with open(input_path, "rb") as f_in:
                with gzip.open(output_path, "wb") as f_out:
                    shutil.copyfileobj(f_in, f_out)

print("Done.")
