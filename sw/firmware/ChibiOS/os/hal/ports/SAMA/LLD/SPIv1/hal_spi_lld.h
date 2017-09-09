/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    hal_spi_lld.h
 * @brief   SAMA SPI subsystem low level driver header.
 *
 * @addtogroup SPI
 * @{
 */

#ifndef HAL_SPI_LLD_H
#define HAL_SPI_LLD_H

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   SPI0 driver enable switch.
 * @details If set to @p TRUE the support for SPI0 is included.
 */
#if !defined(SAMA_SPI_USE_SPI0) || defined(__DOXYGEN__)
#define SAMA_SPI_USE_SPI0              FALSE
#endif
/** @} */
/**
 * @brief   SPI1 driver enable switch.
 * @details If set to @p TRUE the support for SPI1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(SAMA_SPI_USE_SPI1) || defined(__DOXYGEN__)
#define SAMA_SPI_USE_SPI1                 FALSE
#endif


/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/
#if !defined(SAMA_DMA_REQUIRED)
#define SAMA_DMA_REQUIRED
#endif

#if !SAMA_SPI_USE_SPI0 && !SAMA_SPI_USE_SPI1
#error "SPI driver activated but no SPI peripheral assigned"
#endif
/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an SPI driver.
 */
typedef struct SPIDriver SPIDriver;

/**
 * @brief   SPI notification callback type.
 *
 * @param[in] spip      pointer to the @p SPIDriver object triggering the
 *                      callback
 */
typedef void (*spicallback_t)(SPIDriver *spip);

/**
 * @brief   Driver configuration structure.
 */
typedef struct {
  /**
   * @brief Operation complete callback or @p NULL.
   */
  spicallback_t             end_cb;
  /* End of the mandatory fields.*/
  /**
   * @brief The chip select line number.
   */
  uint16_t                  npcs;
  /**
   * @brief SPI MR register initialization data.
   */
  uint32_t                  mr;
  /**
   * @brief SPI CSR register initialization data.
   */
  uint32_t                  csr;
} SPIConfig;

/**
 * @brief   Structure representing an SPI driver.
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
 */
struct SPIDriver {
  /**
   * @brief Driver state.
   */
  spistate_t                state;
  /**
   * @brief Current configuration data.
   */
  const SPIConfig           *config;
#if SPI_USE_WAIT || defined(__DOXYGEN__)
  /**
   * @brief Waiting thread.
   */
  thread_reference_t        thread;
#endif /* SPI_USE_WAIT */
#if SPI_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
  /**
   * @brief Mutex protecting the bus.
   */
  mutex_t                   mutex;
#endif /* SPI_USE_MUTUAL_EXCLUSION */
#if defined(SPI_DRIVER_EXT_FIELDS)
  SPI_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief Pointer to the SPIx registers block.
   */
  Spi                      *spi;
  /**
   * @brief   Receive DMA stream.
   */
  sama_dma_channel_t       *dmarx;
   /**
    * @brief   Transmit DMA stream.
    */
  sama_dma_channel_t       *dmatx;
  /**
    * @brief   RX DMA mode bit mask.
    */
  uint32_t                 rxdmamode;
  /**
    * @brief   TX DMA mode bit mask.
    */
  uint32_t                 txdmamode;
};


/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/
#define SPI_PCS(npcs)       SPI_MR_PCS((~(1 << npcs) & 0xF))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if SAMA_SPI_USE_SPI0 && !defined(__DOXYGEN__)
extern SPIDriver SPID0;
#endif

#if SAMA_SPI_USE_SPI1 && !defined(__DOXYGEN__)
extern SPIDriver SPID1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
void spi_lld_init(void);
  void spi_lld_start(SPIDriver *spip);
  void spi_lld_stop(SPIDriver *spip);
  void spi_lld_select(SPIDriver *spip);
  void spi_lld_unselect(SPIDriver *spip);
  void spi_lld_ignore(SPIDriver *spip, size_t n);
  void spi_lld_exchange(SPIDriver *spip, size_t n,
                        const void *txbuf, void *rxbuf);
  void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf);
  void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf);

#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SPI */

#endif /* HAL_SPI_LLD_H */

/** @} */
