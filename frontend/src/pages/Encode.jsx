import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import Logo from '../components/Logo'
import BackButton from '../components/BackButton'

function Encode() {
  const navigate = useNavigate()
  const [carrierFile, setCarrierFile] = useState(null)
  const [secretFile, setSecretFile] = useState(null)
  const [isLoading, setIsLoading] = useState(false)
  const [error, setError] = useState('')

  const handleCarrierChange = (e) => {
    const file = e.target.files[0]
    if (file) {
      const ext = file.name.split('.').pop().toLowerCase()
      if (ext === 'png') {
        setCarrierFile(file)
        setError('')
      } else {
        setError('Carrier must be PNG')
      }
    }
  }

  const handleSecretChange = (e) => {
    const file = e.target.files[0]
    if (file) {
      setSecretFile(file)
      setError('')
    }
  }

  const handleEncode = async () => {
    if (!carrierFile || !secretFile) {
      setError('Please select both files')
      return
    }

    setIsLoading(true)
    setError('')

    const formData = new FormData()
    formData.append('carrier', carrierFile)
    formData.append('secret', secretFile)

    try {
      const response = await fetch('/api/encode', {
        method: 'POST',
        body: formData,
      })

      if (response.ok) {
        const blob = await response.blob()
        const url = window.URL.createObjectURL(blob)
        navigate('/encode-result', { state: { fileUrl: url, fileName: 'encoded.png' } })
      } else {
        const data = await response.json()
        setError(data.error || 'Encoding failed')
      }
    } catch (err) {
      setError('Network error: ' + err.message)
    } finally {
      setIsLoading(false)
    }
  }

  return (
    <div className="container">
      <BackButton />
      <Logo />
      
      <div className="form-group">
        <label className="label">Carrier</label>
        <input
          type="file"
          id="carrier-input"
          accept=".png"
          onChange={handleCarrierChange}
        />
        <label htmlFor="carrier-input" className={`file-input ${carrierFile ? 'filled' : ''}`}>
          {carrierFile ? carrierFile.name : 'CarrierFile.*ext'}
        </label>

        <label className="label">Secret</label>
        <input
          type="file"
          id="secret-input"
          onChange={handleSecretChange}
        />
        <label htmlFor="secret-input" className={`file-input ${secretFile ? 'filled' : ''}`}>
          {secretFile ? secretFile.name : 'Select File'}
        </label>
      </div>

      <div className="button-group">
        <button 
          className="button primary" 
          onClick={handleEncode}
          disabled={isLoading || !carrierFile || !secretFile}
        >
          {isLoading ? 'Encoding...' : 'Encode'}
        </button>
        <button 
          className="button outline" 
          onClick={() => navigate('/decode')}
        >
          Decode
        </button>
      </div>

      {error && <div className="error">{error}</div>}
    </div>
  )
}

export default Encode
