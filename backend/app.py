from flask import Flask, request, jsonify, send_file
from flask_cors import CORS
import subprocess
import os
import shutil
from pathlib import Path
from datetime import datetime
import tempfile

app = Flask(__name__)
CORS(app)

UPLOAD_FOLDER = Path(__file__).parent / 'uploads'
UPLOAD_FOLDER.mkdir(exist_ok=True)
STEGASAUR_BINARY = Path(__file__).parent.parent / 'stegasaur'

@app.route('/api/health', methods=['GET'])
def health():
    return jsonify({'status': 'ok', 'binary_exists': STEGASAUR_BINARY.exists()})

@app.route('/api/encode', methods=['POST'])
def encode():
    if 'carrier' not in request.files or 'secret' not in request.files:
        return jsonify({'error': 'Missing carrier or secret file'}), 400
    
    carrier_file = request.files['carrier']
    secret_file = request.files['secret']
    
    carrier_ext = Path(carrier_file.filename).suffix
    secret_ext = Path(secret_file.filename).suffix
    
    if carrier_ext.lower() != '.png':
        return jsonify({'error': 'Carrier must be PNG'}), 400
    
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    carrier_path = UPLOAD_FOLDER / f'carrier_{timestamp}{carrier_ext}'
    secret_path = UPLOAD_FOLDER / f'secret_{timestamp}{secret_ext}'
    
    carrier_file.save(carrier_path)
    secret_file.save(secret_path)
    
    try:
        process = subprocess.Popen(
            [str(STEGASAUR_BINARY)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            cwd=str(UPLOAD_FOLDER)
        )
        
        input_data = f"{secret_path}\n{carrier_path}\n"
        stdout, stderr = process.communicate(input=input_data, timeout=30)
        
        output_files = list(UPLOAD_FOLDER.glob('*.png'))
        output_files.sort(key=lambda x: x.stat().st_mtime, reverse=True)
        
        if output_files:
            encoded_file = output_files[0]
            return send_file(
                encoded_file,
                as_attachment=True,
                download_name=f'encoded_{timestamp}.png',
                mimetype='image/png'
            )
        else:
            return jsonify({'error': 'Encoding failed', 'details': stderr}), 500
            
    except subprocess.TimeoutExpired:
        return jsonify({'error': 'Encoding timeout'}), 500
    except Exception as e:
        return jsonify({'error': str(e)}), 500
    finally:
        if carrier_path.exists():
            carrier_path.unlink()
        if secret_path.exists():
            secret_path.unlink()

@app.route('/api/decode', methods=['POST'])
def decode():
    if 'encoded' not in request.files:
        return jsonify({'error': 'Missing encoded file'}), 400
    
    encoded_file = request.files['encoded']
    
    if not encoded_file.filename.endswith('.png'):
        return jsonify({'error': 'Encoded file must be PNG'}), 400
    
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    encoded_path = UPLOAD_FOLDER / f'encoded_{timestamp}.png'
    dummy_secret = UPLOAD_FOLDER / f'dummy_{timestamp}.txt'
    encoded_file.save(encoded_path)
    
    with open(dummy_secret, 'w') as f:
        f.write('dummy')
    
    try:
        process = subprocess.Popen(
            [str(STEGASAUR_BINARY)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            cwd=str(UPLOAD_FOLDER)
        )
        
        input_data = f"{dummy_secret}\n{encoded_path}\n"
        stdout, stderr = process.communicate(input=input_data, timeout=30)
        
        decode_files = list(UPLOAD_FOLDER.glob('decode.*'))
        if decode_files:
            decoded_file = decode_files[0]
            ext = decoded_file.suffix
            return send_file(
                decoded_file,
                as_attachment=True,
                download_name=f'decoded_{timestamp}{ext}'
            )
        else:
            return jsonify({'error': 'Decoding failed', 'details': stderr}), 500
            
    except subprocess.TimeoutExpired:
        return jsonify({'error': 'Decoding timeout'}), 500
    except Exception as e:
        return jsonify({'error': str(e)}), 500
    finally:
        if encoded_path.exists():
            encoded_path.unlink()
        if dummy_secret.exists():
            dummy_secret.unlink()
        for f in UPLOAD_FOLDER.glob('*.png'):
            if f.name.startswith(('Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun')):
                f.unlink()
        for f in UPLOAD_FOLDER.glob('decode.*'):
            f.unlink()

@app.route('/api/cleanup', methods=['POST'])
def cleanup():
    try:
        for f in UPLOAD_FOLDER.glob('*'):
            if f.is_file():
                f.unlink()
        return jsonify({'status': 'cleaned'})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000, debug=True)
